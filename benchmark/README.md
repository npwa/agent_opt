## Test benchmark cases:

Four coding prompts and two language/logic reasoning prompts

1. **coding_01_my_sqrt_review** - In working directory `/home/npalmass/work/OpenCode/claude-files`, review the file `my_sqrt.c` and check if the function correctly calculates the square root of a float number.
   >  expected is a missing check for negative numbers, tolerance does not scale, potential infinite loop

2. **coding_02_fizzbuzz_gen** - Read the requirements in the file `fizzbuzz.md` and create C-code for it. Write to file `fizzbuzz.c`
   >  expected is valid C-code and current execution that matches the given test case

3. **coding_03_compress_gen** - Follow requirements in `compress.md` and produce the C-function
   > expected is a simple string copressor function to test manual memory/pointer handling

4. **coding_04_dir_stats_gen** - Write the bash script requested in `dir_stats.md`
   > expected is a script that shows content stats of a directory path

5. **coding_05_scientific_lan** - What is the current scientific consensus for the MMR vaccine being linked to autism?
   > it turns out that OpenCode can follow links and fetch content, but at this time it has no access to search engine so the expected output of this question is all from training

6. **coding_06_tax_rates_lan** - Explain in less than 150 words why lowering corporate tax rates will not boost long-term job growth
   > designed to test reasoning form training data alone
