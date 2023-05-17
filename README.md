# brightness_sync

用于将NVIDIA显卡的亮度值同步到AMD显卡上。程序使用inotify监控NVIDIA亮度文件的变化，当文件被修改后，程序会读取NVIDIA亮度文件的值，并将其转换为AMD显卡的亮度值，然后将亮度值写入AMD的亮度文件中。

## 可参阅的文档

[Backlight of Arch Wiki](https://wiki.archlinux.org/title/Backlight)