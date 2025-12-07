Order Book Server – Batonics AB Engineering Challenge

This project is a fully self-implemented order book reconstruction & data streaming system, written in modern C++20 and designed following the requirements of the Batonics AB Systems/Infra Engineering Challenge.

Requirement Completion Overview
1. Data Streaming Status: PARTIAL

Supports ingestion of DBN MBO messages

Throughput benchmarks implemented (p50 ≈ 900k msg/s)

Microsecond-level latency measurement for message application

Does not include real TCP streaming from an external 50k–500k msg/s feed

2. Order Book Reconstruction Status: DONE

Fully custom price-level order book engine

Correct handling of ADD / MODIFY / CANCEL operations

FIFO ordering preserved for same-price orders

Snapshot generation latency:

p50 ≈ 0.15 ms

p90 ≈ 0.20 ms

p99 ≈ 0.25 ms

3. Data Storage Status: NOT DONE

No TimescaleDB / ClickHouse / SQLite persistence implemented

4. Deployment (Dockerized) Status: DONE

Dockerfile + docker-compose provided

Deployable on AWS EC2

Supports NGINX / Cloudflare proxy setups

5. API Layer (REST / WebSocket) Status: DONE (REST), NOT DONE (WebSocket)

REST endpoints: /snapshot, /metrics

WebSocket live feed not implemented

6. Frontend – Live Visualization Status: DONE

Displays real-time bids/asks

Auto-refresh loop

Latency + throughput percentiles shown on UI

7. Configuration Management Status: PARTIAL

Supports environment variables

No multi-stage configuration system

8. Reproducible Builds / CI Status: DONE

GitHub Actions pipeline

Automatic build + unit testing per commit

9. Testing Status: DONE

23+ unit tests covering:

Add / Modify / Cancel

FIFO behavior

Complex multi-step transitions

All tests integrated into CI

10. Performance Optimization Status: PARTIAL

p99 snapshot latency < 1ms

p99 apply_mbo < 10μs

No SIMD / multithread optimization yet

11. Observability (latency, throughput) Status: DONE

Latency p50/p90/p99 for snapshots + MBO processing

Throughput p50/p90/p99 (msgs/s)

Returned via /metrics API and displayed in UI

12. Infrastructure as Code Status: NOT DONE

No Terraform / Pulumi scripts

13. Multi-Environment Setup Status: NOT DONE

No dev/staging/prod separation

14. Resilience Testing Status: NOT DONE

No fault injection tests

No restart / reconnection testing

15. API Reliability Status: PARTIAL

Error handling for REST

No retry / idempotency logic

16. Security Status: NOT DONE

HTTPS supported via nginx

No dependency scanning / SBOM

17. Correctness Verification Status: DONE

Correct price-time priority ensured