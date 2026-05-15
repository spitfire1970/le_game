clang-format -i main.cpp && \
clang++ main.cpp -o game \
-I/opt/homebrew/include \
-L/opt/homebrew/lib \
-lraylib \
-framework CoreVideo -framework IOKit -framework Cocoa -framework OpenGL \
&& ./game
