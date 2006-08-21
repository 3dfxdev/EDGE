# 
# MAKEFILE
#

all:
	cd main  && htp -quiet @main_build.rsp
	cd shots && htp -quiet @shots_build.rsp
	cd ddf   && htp -quiet @ddf_build.rsp
	cd rts   && htp -quiet @rts_build.rsp
	cp css/* out/css
	cp images/* out/images
	cp shots/*.jpg shots/*.png out/shots
	cp main/README.txt out

clean:
	rm -f out/*.htm out/*.txt
	rm -f out/shots/*
	rm -f out/ddf_online/*.htm
	rm -f out/rts_online/*.htm
	rm -f out/css/*.css
	rm -f out/images/*

zip:
	cd .. && zip -rl web_page.zip web_page
	cd .. && zip -r  web_page.zip web_page -i \*.exe \*.jpg \*.png

.PHONY: all clean zip

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
