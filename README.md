# OneMessageCPP

C++ package for [OneMessage](https://github.com/Jiu-xiao/OneMessage.git)

---

## USAGE

Please refer to OneMessage [Readme.md](https://github.com/Jiu-xiao/OneMessage/blob/master/README.md).

CMakeList.txt

    add_subdirectory(OneMessageCPP_directory)

    target_include_directories(
        OneMessage
        PUBLIC om_config_file_directory
        ...
    )
