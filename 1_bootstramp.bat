@echo off

if not exist "vcpkg" (
    git clone https://github.com/microsoft/vcpkg.git
)

cd vcpkg

git reset --hard 2145fbe4f0a08de4547f391de47d88c18a01f1ba

if not exist "vcpkg.exe" (
    bootstrap-vcpkg.bat
)

