#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <cstring>
#include <functional>

class MyVirtualKeyboard
{
public:
    explicit MyVirtualKeyboard(const std::string &event_path)
    {
        fd_event = open(event_path.c_str(), O_RDONLY);
        fd_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd_event == -1 || fd_uinput == -1)
        {
            throw std::runtime_error("open failed");
        }
        try
        {
            check();
            setup();
        }
        catch (...)
        {
            cleanUp();
            throw;
        }
    }

    ~MyVirtualKeyboard()
    {
        cleanUp();
    }

    using CustomEventCallback = std::function<void(const input_event &)>;

    void run(const CustomEventCallback &callback)
    {
        struct input_event ie;
        while (read(fd_event, &ie, sizeof(input_event)) > 0)
        {
            if (ie.type == EV_KEY)
            {
                std::cout << "Key event read: type=" << ie.type << " code=" << ie.code << " value=" << ie.value << std::endl;
                callback(ie);
            }
        }
    }

    void emit(int type, int code, int val)
    {
        struct input_event ie;
        memset(&ie, 0, sizeof(struct input_event));
        ie.type = type;
        ie.code = code;
        ie.value = val;
        std::cout << "Key event emitted: type=" << type << " code=" << code << " value=" << val << std::endl;
        write(fd_uinput, &ie, sizeof(struct input_event));
        // シンクイベントを送信
        memset(&ie, 0, sizeof(struct input_event));
        ie.type = EV_SYN;
        ie.code = SYN_REPORT;
        ie.value = 0;
        write(fd_uinput, &ie, sizeof(struct input_event));
        std::cout << "Sync event emitted" << std::endl;
    }

private:
    int fd_event = -1, fd_uinput = -1;

    void check()
    {
        if (ioctl(fd_event, EVIOCGRAB, 1) < 0)
        {
            throw std::runtime_error("ioctl EVIOCGRAB failed");
        }
    }

    void setup()
    {
        uinput_setup usetup;
        memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor = 0x1234;
        usetup.id.product = 0x5678;
        strcpy(usetup.name, "MyVirtualKeyboard");
        // 仮想デバイスがイベントタイプであるEV_KEYの発行を許可
        ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);
        // 仮想デバイスが全キーのイベント発行を許可
        for (int key = 0; key < 256; key++)
        {
            ioctl(fd_uinput, UI_SET_KEYBIT, key);
        }
        ioctl(fd_uinput, UI_DEV_SETUP, &usetup);
        ioctl(fd_uinput, UI_DEV_CREATE);
    }

    void cleanUp()
    {
        if (fd_event != -1)
        {
            ioctl(fd_event, EVIOCGRAB, 0);
            close(fd_event);
        }
        if (fd_uinput != -1)
        {
            ioctl(fd_uinput, UI_DEV_DESTROY);
            close(fd_uinput);
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_device>" << std::endl;
        return 1;
    }

    std::string event_path = argv[1];

    try
    {
        MyVirtualKeyboard keyboard(event_path);
        keyboard.run([&keyboard](const input_event &ie) {
            if (ie.type == EV_KEY)
            {
                std::cout << "Key event: code=" << ie.code << " value=" << ie.value << std::endl;
                if (ie.code == KEY_A)
                {
                    // AキーをSキーに変換して送信
                    keyboard.emit(ie.type, KEY_S, ie.value);
                }
                else
                {
                    // その他のキーイベントはそのまま送信
                    keyboard.emit(ie.type, ie.code, ie.value);
                }
            }
        });
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
