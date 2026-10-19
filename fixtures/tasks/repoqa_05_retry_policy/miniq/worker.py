"""Executes a single job attempt."""
import logging

log = logging.getLogger("miniq")


def run_job(queue, fn, args, attempt):
    try:
        return fn(*args)
    except Exception as exc:  # noqa: BLE001
        if queue.policy.should_retry(attempt, exc):
            log.warning("job failed (attempt %d): %s; retrying", attempt, exc)
            queue.requeue(fn, args, attempt)
        else:
            log.error("job failed permanently after %d attempts: %s", attempt, exc)
