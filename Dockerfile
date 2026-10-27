# One-shot build:
#   docker build --output out .
# extracts out/research_kmem.ko to ./out on the host.
# (Fallback: docker build -t kmod-recipe . && docker create --name k kmod-recipe
#  && docker cp k:/research_kmem.ko out/ && docker rm k)
FROM debian:bookworm-slim AS builder
RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc-arm-none-eabi binutils-arm-none-eabi python3 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN chmod +x build.sh verify.sh && ./build.sh

FROM scratch
COPY --from=builder /src/out/research_kmem.ko /research_kmem.ko
