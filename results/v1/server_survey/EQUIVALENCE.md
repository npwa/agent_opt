# Is a private Ollama server with default settings equivalent to the system service?

The baselines ran on the system service (unknown environment); the sweep configurations run on a private
`ollama serve` started by the runner. `equiv_check.py` compares them on this box (`qwen3.6-32k`, 32k context):

| Measure | System service | Private server (`sw_32k`, explicit defaults) |
|---|---:|---:|
| cold first call, 7.4k-token prompt (prompt replay, nonce) | 31.25 s (3 repeats, `results/v1/replay_probe`) | 31.04 s, 31.12 s |
| warm first call | 2.50 s | 2.48 s, 2.46 s |
| decode speed, 300-token generation (tok/s) | 33.3, 33.3, 33.3 | 31.7, 35.6, 35.8 |

Prefill agrees within 1 %. Decode differs by at most about 7 % in either direction with three samples each
(the first private sample includes warm-up), which is within what the repeats show; no systematic difference
is claimed. So `sw_*` configurations are comparable with the earlier baselines; the one remaining difference is
that the system service's flash-attention setting is not visible to us (the private default logs `auto`).
