#pragma once

struct Process {
  virtual ~Process() = default;

  /** Idempotent run. */
  virtual void run() = 0;

  virtual bool wait() = 0;
};
