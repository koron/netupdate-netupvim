@echo off

if "%1" == "" goto ERROR

.\http.exe http://www.kaoriya.net/testdir/proxytest.html -o download.log -p %1 > execute.log
goto END

:ERROR
echo netupvim�v���L�V���쌟�؃v���O����
echo   �g����:  dotest.bat {�v���L�V��URL��������IP}[:�v���N�V�̃|�[�g�ԍ�]
echo   ��:      dotest.bat proxy.kaoriya.net:8080
echo ���s�������ʂ�execute.log�����download.log�ɏo�͂���܂��B
echo �ڂ�������README.html���Q�Ƃ��Ă��������B
:END
