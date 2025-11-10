 # I will not compile angle in this project

It takes too long, too complicated toolchain...

In your workspace, were all your projects are:

    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    export PATH="$PWD/depot_tools:$PATH"

Then create a new angle project, instead of git use:

    mkdir ../angle
    cd ../angle
    fetch angle
    gclient sync

Then build it like this:

    gn gen out/Release 
    ninja - out/Release

Then take the files and use them in your build:

    cp out/Release/*.dylib ../bowling/3rdparty/gles4mac/lib/

You may need to update xcode and make sure you are not in nix-shell.