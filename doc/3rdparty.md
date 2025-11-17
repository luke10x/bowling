External Library Management
===========================

External libraries are added to this repo as modules in ./external directory.

Getting the source code of external modules 
-------------------------------------------

If the Vortex Boilerplate is already cloned,
run this in in its root directory:

    git submodule init
    git submodule update

Adding a new library
--------------------

1. Add library repo as a new module:

    git submodule add https://github.com/libsdl-org/SDL.git external/SDL

2. Go into the library directory:

    cd ./3rdparty/SDL

Skip to step 4 if you don't want to use tags.

3. See available tags:

    git tag

4. Checkout your favorite :

    git checkout tags/release-2.30.8

or (if you dont want to pin to a tag):

    git checkout main

5. now go back to the repository root:

    cd ../../

6. Add and commit the library checked-out version:

    git add external/SDL
    git commit -m "Add SDL submodule at tag release-2.30.8"

Listing installed libraries
---------------------------

When replicating this configuration to another project list what is installed:

    git submodule status

To see their repo urls:
    
    cat path_to_old_repo/.gitmodules


Now patiently add every module to your new ./external one by one following the above instructions
Steps: 1, 2, 4, 5, (6 - just do it at the end of everything)

