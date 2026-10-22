# Status: unconfined run, partly contaminated

These `qwen3:8b` trials ran before the agent was confined (plan item P14). In the `refactor_02_rename_price` trial of this
directory the model searched the filesystem for `calc_price` and **edited the repository's own fixture and golden files**
(restored with `git checkout`); the same happened in two trials of `../q3_8b_core_n3/`. Those three trials, and any
`refactor_02` trial that ran after a fixture was modified, are invalid. The other trials never touched a path outside
their working directory (checked by searching every events file for repository paths), so they are usable as a first,
provisional picture, but the whole screen should be repeated under confinement before any conclusion is drawn.
