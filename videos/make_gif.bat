ffmpeg -i fem_1_cut.mkv -r 15 -filter "setpts=PTS*0.5,scale=120:-2" fem_1_cut_gif_src.mp4 -y
ffmpeg -i fem_1_cut_gif_src.mp4 -filter_complex "[0:v] palettegen" palette_gif_src.png -y
ffmpeg -i fem_1_cut_gif_src.mp4 -i palette.png -filter_complex "[0:v][1:v] paletteuse" fem_1_cut.gif -y