"""Retry/backoff policy for failed jobs."""
import random

from . import config


class RetryPolicy:
    def __init__(self, max_attempts=None, base_delay=None, max_delay=None, jitter=None):
        self.max_attempts = max_attempts or config.RETRY_MAX_ATTEMPTS
        self.base_delay = base_delay or config.RETRY_BASE_DELAY_S
        self.max_delay = max_delay or config.RETRY_MAX_DELAY_S
        self.jitter = config.RETRY_JITTER if jitter is None else jitter

    def should_retry(self, attempt, exc):
        """attempt is 1-based; permanent errors are never retried."""
        if isinstance(exc, PermanentError):
            return False
        return attempt < self.max_attempts

    def next_delay(self, attempt):
        """Exponential backoff: base * 2**(attempt-1), capped, with +/- jitter."""
        raw = self.base_delay * (2 ** (attempt - 1))
        capped = min(raw, self.max_delay)
        spread = capped * self.jitter
        return max(0.0, capped + random.uniform(-spread, spread))


class PermanentError(Exception):
    """Raised by handlers to signal that retrying cannot help."""
