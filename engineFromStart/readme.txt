1.	在windows下编译openssl
使用vs的命令行工具编译，参照install说明

2.	编写简单的engine测试
代码见windowsEngineTest文件夹下silly-engine.c（来自openssl官方blog）。
1）	使用vs的 命令行工具；
2）	编译生成silly.obj文件（编译命令由windows下的openssl makefile得到）：
cl /Fosilly.obj /I \openssl\include /MD /Ox /O2 /Ob2 -DOPENSSL_THREADS  -DDSO_WIN32 -W3 -Gs0 -GF -Gy -nologo -DOPENSSL_SYSNAME_WIN32 -DWIN32_LEAN_AND_MEAN -DL_ENDIAN -D_CRT_SECURE_NO_DEPRECATE -DOPENSSL_USE_APPLINK -DOPENSSL_NO_RC5 -DOPENSSL_NO_MD2 -DOPENSSL_NO_KRB5 -DOPENSSL_NO_JPAKE -DOPENSSL_NO_STATIC_ENGINE /Zi -D_WINDLL -DOPENSSL_BUILD_SHLIBSSL -c silly-engine.c
3）	连接：
link /dll /out:silly.dll silly.obj \openssl\out32dll\libeay32.lib
4）使用动态engine载入silly.dll：
openssl engine -v dynamic -pre "SO_PATH:C:\study\engineFromStart\1windowsEngineTest\silly.dll" -pre LOAD
  运行结果：
(dynamic) Dynamic engine loading support
[Success]: SO_PATH:C:\study\engineFromStart\1windowsEngineTest\silly.dll
[Success]: LOAD
Loaded: (silly) A silly engine for demonstration purposes

3.	测试SKF
cl /Fotestdso.obj /I \openssl\include /I .\skf /MD /Ox /O2 /Ob2 -DOPENSSL_THREADS  -DDSO_WIN32 -W3 -Gs0 -GF -Gy -nologo -DOPENSSL_SYSNAME_WIN32 -DWIN32_LEAN_AND_MEAN -DL_ENDIAN -D_CRT_SECURE_NO_DEPRECATE -DOPENSSL_USE_APPLINK -DOPENSSL_NO_RC5 -DOPENSSL_NO_MD2 -DOPENSSL_NO_KRB5 -DOPENSSL_NO_JPAKE -DOPENSSL_NO_STATIC_ENGINE /Zi -D_WINDLL -DOPENSSL_BUILD_SHLIBSSL -c e_testdso.c



