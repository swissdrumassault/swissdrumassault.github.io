all: build serve

build:
	ablog build -w blog

serve:
	ablog serve -w blog -p 8080
