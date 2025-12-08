# Order Book Server – Batonics AB Engineering Challenge
This project is a fully self-implemented **order book reconstruction & data streaming system**, written in modern **C++20** and designed following the requirements of the **Batonics AB Systems/Infra Engineering Challenge**.

---

## Web UI
`Web UI` is deployed at: [https://tam-tradingengine-test.com](https://tam-tradingengine-test.com/).

---

## Requirement Completion Overview

### 1. Data Streaming — **Status: PARTIAL**
- Supports ingestion of DBN MBO messages: class ([`dbn_wrapper.h`](src/dbn_wrapper/dbn_wrapper.h))
- Throughput benchmarks implemented (p50 ≈ **900k msg/s**)
- Microsecond-level latency measurement for message application
- **Does not** include real TCP streaming from an external 50k–500k msg/s feed

---

### 2. Order Book Reconstruction — **Status: DONE**
- Fully custom price-level order book engine: class ([`orderbook.h`](src/orderbook/orderbook.h))
- Correct handling of **ADD / MODIFY / CANCEL** operations
- FIFO ordering preserved for same-price orders
- Snapshot generation latency:
  - **p50 ≈ 0.21 ms**
  - **p90 ≈ 0.59 ms**
  - **p99 ≈ 0.65 ms**

---

### 3. Data Storage — **Status: NOT DONE**
- No TimescaleDB / ClickHouse / SQLite persistence implemented
- No historical query layer

---

### 4. Deployment (Dockerized) — **Status: DONE**
- Fully Dockerized system with reproducible builds: ([`Dockerfile`](z_docker/Dockerfile)) + ([`docker-compose.yaml`](z_docker/docker-compose.yaml))
- One-command startup for server + frontend

---

### 5. API Layer (REST / WebSocket) — **Status: DONE (REST)**
- REST endpoints implemented:
  1. Get snapshot: [/get_snapshot](https://github.com/huutam1991/order_book_server_cpp/blob/8270ee18e18810e403862e20cc5984e7946b16e8/src/main.cpp#L22-L32)
  2. Start streaming orderbook: [/start_streaming_orderbook](https://github.com/huutam1991/order_book_server_cpp/blob/8270ee18e18810e403862e20cc5984e7946b16e8/src/main.cpp#L34-L55),
  3. Stop streaming orderbook: [/stop_streaming_orderbook](https://github.com/huutam1991/order_book_server_cpp/blob/8270ee18e18810e403862e20cc5984e7946b16e8/src/main.cpp#L57-L66)
- Support 10 - 100 concurrent clients reading the order book

---

### 6. Frontend – Live Visualization — **Status: DONE**
- Web UI visualizing real-time order book updates: [https://tam-tradingengine-test.com](https://tam-tradingengine-test.com/)
- Displays depth, price levels, snapshot latency, and MBO apply latency
- Clean dark-themed layout for reading trading data

---

### 7. Configuration Management — **Status: PARTIAL**
- Basic config handled via environment variables: `LOG_LEVEL` + `PROD` ([`docker-compose.yaml`](https://github.com/huutam1991/order_book_server_cpp/blob/8270ee18e18810e403862e20cc5984e7946b16e8/z_docker/docker-compose.yaml#L31-L35))
- No layered config system (YAML/TOML/JSON with overrides)

---

### 8. Reproducible Builds / CI — **Status: DONE**
- GitHub Actions pipeline created (for an example: [CI for commit #19](https://github.com/huutam1991/order_book_server_cpp/actions/runs/20021925172/job/57410491458))
- Every commit triggers:
  1. Dependency installation
  2. Full build
  3. Unit test execution
- Build environment is fully deterministic

---

### 9. Testing — **Status: DONE**
- 23 unit tests (Google Test) across ADD / MODIFY / CANCEL behavior: ([`test_orderbook/`](test_cases/src/test_orderbook))
- Validates FIFO ordering, size adjustments, price movements
- Runs automatically in CI

---

### 10. Performance Optimization (p99 < 10ms) — **Status: DONE**
- Snapshot generation meets target (p99 ≈ **0.65 ms**)
- MBO apply latency:
  - p50 ≈ **1.1–1.2 µs**
  - p90 ≈ **2.4–2.6 µs**
  - p99 ≈ **3.5–4.3 µs**
- Throughput:
  - p50 ≈ **900k msg/s**
  - p90 ≈ **450k msg/s**
  - p99 ≈ **250k msg/s**

---

### 11. Observability — **Status: DONE**
- Real-time measurement of:
  - Snapshot latency
  - MBO apply latency
  - Throughput percentile metrics
- Metrics displayed in frontend UI

---

### 12. Infrastructure as Code — **Status: NOT DONE**
- No Terraform / Pulumi / Ansible / Helm
- No automated provisioning for cloud environments

---

### 13. Multi-Environment Setup — **Status: NOT DONE**
- No `dev/staging/prod` separation
- All builds run in the same environment

---

### 14. Resilience Testing — **Status: NOT DONE**
- No simulation of:
  - Connection drops
  - Message gaps
  - High-latency injectors
  - Randomized input mutation

---

### 15. API Reliability — **Status: PARTIAL**
- REST API stable under moderate load
- No retry logic, no idempotency keys, no rate limiters
- No fault injection tests

---

### 16. Security — **Status: NOT DONE**
- No SBOM
- No dependency scanning
- No API authentication

---

### 17. Correctness Verification — **Status: DONE**
- Order book engine validated through comprehensive unit tests
- FIFO compliance, cancel edge cases, multi-level modification sequences
- Ensures no violation of exchange semantics

---

## Build (Linux)

```
./build_bash.sh
```

## Run (Linux)

```
./run_bash.sh
```

---

## Final Note

This implementation successfully demonstrates **order book reconstruction, real-time performance metrics, REST API exposure, CI/CD, and frontend visualization**, covering the majority of the Batonics AB engineering challenge requirements.

