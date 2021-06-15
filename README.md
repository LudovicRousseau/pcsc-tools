pcsc-tools
==========

Project home page
-----------------
http://ludovic.rousseau.free.fr/softwares/pcsc-tools/

To get a ``.tar.bz2`` archive of the project please go to http://ludovic.rousseau.free.fr/softwares/pcsc-tools/

Source code
-----------
You can use this project to report pull requests.

How to build
-----------
The source code is distributed in maintainer mode, rather than developer mode.
Ths should probably be fixed, but for now, start building by:

	sudo apt install libpcsclite-dev
	# also call pcsc-lite-devel in some environments

	autoreconf -i
	./configure
	make
