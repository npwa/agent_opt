"""Default settings for miniq."""

DEFAULT_WORKERS = 4
POLL_INTERVAL_S = 0.25

# Retry policy defaults (see retry.py)
RETRY_MAX_ATTEMPTS = 5
RETRY_BASE_DELAY_S = 0.5
RETRY_MAX_DELAY_S = 30.0
RETRY_JITTER = 0.1

# Jobs older than this are dropped by the reaper
JOB_TTL_S = 3600
