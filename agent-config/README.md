# Agent and other configuration files for this setup

|Filename      | Purpose
|--------------|-
|Modelfile     | Variant of qwen3.6 with quantization for 32k context window.
|Modelfile2    | Variant of qwen3.6 with quantization for 16k context window.
|Modelfile3    | Variant of qwen3.6 with quantization for 64k context window.
|opencode.json | `OpenCode` configuration. Only one model can be named, and MCP servers are added.
|settings.yml  | `SearXNG` configuration, specific for Intel network environment.
|stack\_architecture.png | Outline of connections between OpenCode, Ollama and SearXNG. Ollama Web UI was listening on port local 8080.
|agent         | Custom system prompts for specific types of tasks.
|comm-tools    | MCP server exposing sanbox tools send\_email and create\_event as agent tools
