"""Job queue with worker threads."""
import heapq
import threading
import time

from . import config
from .retry import RetryPolicy
from .worker import run_job


class JobQueue:
    def __init__(self, workers=config.DEFAULT_WORKERS, policy=None):
        self.workers = workers
        self.policy = policy or RetryPolicy()
        self._heap = []
        self._lock = threading.Lock()
        self._seq = 0

    def submit(self, fn, *args, delay=0.0):
        with self._lock:
            self._seq += 1
            heapq.heappush(self._heap, (time.monotonic() + delay, self._seq, fn, args, 1))

    def _pop_ready(self):
        with self._lock:
            if self._heap and self._heap[0][0] <= time.monotonic():
                return heapq.heappop(self._heap)
        return None

    def requeue(self, fn, args, attempt):
        delay = self.policy.next_delay(attempt)
        with self._lock:
            self._seq += 1
            heapq.heappush(self._heap, (time.monotonic() + delay, self._seq, fn, args, attempt + 1))

    def drain(self):
        """Run until the queue is empty (single-threaded helper for tests)."""
        while self._heap:
            item = self._pop_ready()
            if item is None:
                time.sleep(config.POLL_INTERVAL_S)
                continue
            _, _, fn, args, attempt = item
            run_job(self, fn, args, attempt)
