// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.application;

import java.time.Instant;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;

/**
 * Reindexing status for each document type in a content cluster.
 *
 * @author jonmv
 */
public class ClusterReindexing {

    private static final ClusterReindexing empty = new ClusterReindexing(Map.of());

    private final Map<String, Status> documentTypeStatus;

    public ClusterReindexing(Map<String, Status> documentTypeStatus) {
        this.documentTypeStatus = Map.copyOf(documentTypeStatus);
    }

    public static ClusterReindexing empty() { return empty; }

    public Map<String, Status> documentTypeStatus() { return documentTypeStatus; }


    public static class Status {

        private final Instant startedAt;
        private final Instant endedAt;
        private final State state;
        private final String message;
        private final String progress;

        public Status(Instant startedAt, Instant endedAt, State state, String message, String progress) {
            this.startedAt = Objects.requireNonNull(startedAt);
            this.endedAt = endedAt;
            this.state = state;
            this.message = message;
            this.progress = progress;
        }

        public Instant startedAt() { return startedAt; }
        public Optional<Instant> endedAt() { return Optional.ofNullable(endedAt); }
        public Optional<State> state() { return Optional.ofNullable(state); }
        public Optional<String> message() { return Optional.ofNullable(message); }
        public Optional<String> progress() { return Optional.ofNullable(progress); }
    }


    public enum State {

        PENDING, RUNNING, FAILED, SUCCESSFUL;

    }
}