del otp.obj
dv10-elf-objcopy.exe -O binary  main.or32 otp.obj
ac-download.exe -f otp.obj 0x8000 -dev jielijtag
