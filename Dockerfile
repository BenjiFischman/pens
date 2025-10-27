# Multi-stage build for PENS (Professional Email Notification System)

# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY include/ /build/include/
COPY src/ /build/src/
COPY Makefile /build/

# Build the application
RUN make clean && make release

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for security
RUN useradd -m -u 1000 pens && \
    mkdir -p /app/logs && \
    chown -R pens:pens /app

# Set working directory
WORKDIR /app

# Copy binary from builder
COPY --from=builder /build/pens /app/pens

# Copy configuration template
COPY config/pens.conf.example /app/config/pens.conf.example

# Set ownership
RUN chown -R pens:pens /app

# Switch to non-root user
USER pens

# Expose any ports if needed (not required for IMAP client)
# EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD [ -f /app/pens.log ] || exit 1

# Set environment variables with defaults
ENV PENS_IMAP_SERVER=imap.gmail.com \
    PENS_IMAP_PORT=993 \
    PENS_IMAP_USE_SSL=true \
    PENS_PRIORITY_THRESHOLD=5 \
    PENS_CHECK_INTERVAL=60 \
    PENS_DEBUG_MODE=false \
    PENS_LOG_LEVEL=INFO

# Volume for logs
VOLUME ["/app/logs"]

# Run the application
ENTRYPOINT ["/app/pens"]
CMD []

