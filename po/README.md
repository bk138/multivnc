# Add or Update Translation

1. Run `xgettext --keyword=_ --sort-output --package-name=MultiVNC ../src/*.cpp ../src/gui/*.cpp -o multivnc.pot`
   from this directory which will create a `multivnc.pot` translation template file.
2. Use poedit or a similar tool to create or update a translation, save the resulting .po file in this directory.
     - Make sure to add a `translator-credits` entry as in the other .po files so your name automatically shows up in the About dialog. 
4. Adjust CMakeLists.txt to build and install the corresponding .mo file.
