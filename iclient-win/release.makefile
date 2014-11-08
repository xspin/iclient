
FILES = client_win.exe\
		client.exe\
		run.bat

all:
	cp client_win.exe ./release/iClient.exe
	cp client.exe ./release/Info.exe
	#cp run.bat ./release/run.bat


