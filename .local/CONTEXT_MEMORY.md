# Context Memory

Keep durable project decisions here.

## Purpose

- Capture decisions that affect architecture, constraints, and workflow.
- New developers must read this first.

## Update rules

- Update only when a decision is made.
- Include date, decision, and impact.

## Current decisions

- WSL2-first optimization is primary.
- No busy loops; bounded queues; backpressure; deterministic under load.
- Unix domain socket IPC.
- No thread-per-request; clean shutdown; minimal dependencies.
- Only manage/terminate processes that are explicitly heidi-managed.
