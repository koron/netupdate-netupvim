@echo off

if "%1" == "" goto ERROR

.\http.exe http://www.kaoriya.net/testdir/proxytest.html -o download.log -p %1 > execute.log
goto END

:ERROR
echo netupvimプロキシ動作検証プログラム
echo   使い方:  dotest.bat {プロキシのURLもしくはIP}[:プロクシのポート番号]
echo   例:      dotest.bat proxy.kaoriya.net:8080
echo 実行した結果はexecute.logおよびdownload.logに出力されます。
echo 詳しい情報はREADME.htmlを参照してください。
:END
