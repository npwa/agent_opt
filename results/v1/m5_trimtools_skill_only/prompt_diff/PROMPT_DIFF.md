# Prompt diff: default vs deny task, todowrite, skill, webfetch

Captured with a stub model server (no GPU), OpenCode 1.18.11, isolated config as in the runner.

- tools default: ['bash', 'edit', 'glob', 'grep', 'read', 'skill', 'task', 'todowrite', 'webfetch', 'write']
- tools trimmed: ['bash', 'edit', 'glob', 'grep', 'read', 'write']
- system prompt: 9646 -> 8949 characters
- tool schema characters removed: 8648
- identical non-system, non-tool request fields: True

## System prompt diff

```diff
--- default
+++ trimmed
@@ -100 +100 @@
-  Working directory: /tmp/claude-1001/-home-npalmass-work-agent-opt/c564fb3b-05d3-4ebd-b89f-23c064a404ed/scratchpad/stub/w_base/work
+  Working directory: /tmp/claude-1001/-home-npalmass-work-agent-opt/c564fb3b-05d3-4ebd-b89f-23c064a404ed/scratchpad/stub/w_perm_deny/work
@@ -106,9 +105,0 @@
-Skills provide specialized instructions and workflows for specific tasks.
-Use the skill tool to load a skill when a task matches its description.
-<available_skills>
-  <skill>
-    <name>customize-opencode</name>
-    <description>Use ONLY when the user is editing or creating opencode's own configuration: opencode.json, opencode.jsonc, files under .opencode/, or files under ~/.config/opencode/. Also use when creating or fixing opencode agents, subagents, skills, plugins, MCP servers, or permission rules. Do not use for the user's own application code, or for any project that is not configuring opencode itself.</description>
-    <location>&lt;built-in&gt;</location>
-  </skill>
-</available_skills>
```
