<!---
    name: memperftest
    platform: WDM
    language: c
    category: General
    description: Tests the performance of the Windows kernel memory API.
--->

This test consists of a test application (testapp.exe) and driver (memperftest.sys)

testapp.exe calls into memperftest.sys to allocate memory and check the execution time.
It takes in the following arguments:
- NUM_TESTS - Number of times to run the performance test. Execution times are averaged at the end. 
- NON_PAGED_MEMORY_SIZE_IN_BYTES (Optional, defaults to 14400000) - Size of the non-paged memory chunk to allocate. This is allocated before the contiguous chunks.
- CONTIGUOUS_MEMORY_SIZE_IN_BYTES (Optional, defaults to 1023) - Size of the contiguous memory chunks to allocate.
- NUM_CONTIGUOUS_MEMORY_CHUNKS (Optional, defaults to 3500) - Number of contiguous memory chunks to allocate.

The defaults and order of allocation were chosen to represent a real application.


Run the test
--------------

To test this driver:
- Copy testapp.exe and memperftest.exe to the same directory. 
- Ensure testsigning and nointegritychecks are configured to "on" via bcdedit
- Ensure proper runtimes are installed
- Execute testapp.exe in an elevated command prompt (run without arguments to see usage)

