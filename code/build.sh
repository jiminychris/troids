Release=0

if [ ! -d "~/Projects/build" ]; then
    mkdir ~/Projects/build
fi
pushd ~/Projects/build

if [ $Release = 1 ]; then
    Internal=0
    DebugDisplay=0
    Slow=0
    Rumble=1
    Optimize=-O3
else
    Internal=1
    DebugDisplay=1
    Slow=1
    Rumble=0
    Optimize=-O0
fi

DataPath=\"../troids/data\"

CommonCompilerFlags="$Optimize -DONE_FILE=0 -DTROIDS_RUMBLE=$Rumble -DTROIDS_INTERNAL=$Internal -DTROIDS_SLOW=$Slow -DTROIDS_DEBUG_DISPLAY=$DebugDisplay -DDATA_PATH=$DataPath -g -Wextra -Werror -Wno-c++11-extensions -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -Wno-null-dereference -Wno-c++11-compat-deprecated-writable-strings -Wno-missing-field-initializers -Wno-switch -fno-exceptions -fno-rtti -stdlib=libc++"

clang++ $CommonCompilerFlags -fno-objc-arc ~/Projects/troids/code/osx_troids.mm -framework Cocoa -framework CoreVideo -framework OpenGL -o osx_troids 

popd
