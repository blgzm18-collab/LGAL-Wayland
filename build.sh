# 2. Create build directory and configure
cmake -B build -S .

# 3. Compile the library and test sandbox
cmake --build build

# 4. Run the test sandbox executable
./build/lgal_sandbox