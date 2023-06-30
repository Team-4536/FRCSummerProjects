:: build gl/stb (uncomment if you need a rebuild)
g++ -c external/glad/gl.c -Iexternal/glad -o out/gl.o -g
g++ -c external/stb_image/stb_image.c -Iexternal/stb_image -o out/stb_image.o -g
g++ -c external/stb_truetype/stb_truetype.c -Iexternal/stb_truetype -o out/stb_truetype.o -g