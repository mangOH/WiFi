localhost: app.localhost

ar7: app.ar7

wp7: app.wp7

ar86: app.ar86

wp85: app.wp85

app.%:
	mkapp wifiService.adef -i ./../ -i platformAdaptor/inc/ -t $* -v

clean:
	rm -rf _build_* *.localhost *.ar7 *.wp7 *.ar8 *.wp85 *.update
