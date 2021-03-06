// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "i_document_scan_iterator.h"

namespace proton {

struct IDocumentMetaStore;

/**
 * Iterator for scanning all documents in a document meta store.
 */
class DocumentScanIterator : public IDocumentScanIterator
{
private:
    const IDocumentMetaStore &_metaStore;
    document::GlobalId        _lastGid;
    bool                      _lastGidValid;
    bool                      _itrValid;

public:
    DocumentScanIterator(const IDocumentMetaStore &_metaStore);

    // Implements IDocumentScanIterator
    virtual bool valid() const override;

    virtual search::DocumentMetaData next(uint32_t compactLidLimit,
                                          uint32_t maxDocsToScan,
                                          bool retry) override;
};

} // namespace proton

