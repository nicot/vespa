// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "fieldpositionsiterator.h"
#include "fieldinfo.h"
#include <vespa/searchlib/common/feature.h>
#include <cstring>
#include <cassert>

class MatchDataHeapTest;

namespace search::fef {

class TermMatchDataMerger;

/**
 * Match information for a single term within a single field.
 **/
class TermFieldMatchData
{
public:
    typedef const TermFieldMatchDataPosition * PositionsIterator;
    typedef TermFieldMatchDataPosition * MutablePositionsIterator;
    struct Positions {
        uint16_t                    _maxElementLength;
        uint16_t                    _allocated;
        TermFieldMatchDataPosition *_positions;
    } __attribute__((packed));

    union Features {
        feature_t     _rawScore;
        unsigned char _position[sizeof(TermFieldMatchDataPosition)];
        Positions     _positions;
        uint64_t      _subqueries;
    } __attribute__((packed));
private:
    bool  isRawScore()  const { return _fieldId & 0x8000; }
    bool  isMultiPos()  const { return _fieldId & 0x4000; }
    bool  empty() const { return _sz == 0; }
    void  clear() { _sz = 0; }
    bool  allocated() const { return isMultiPos(); }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    const TermFieldMatchDataPosition * getFixed() const { return reinterpret_cast<const TermFieldMatchDataPosition *>(_data._position); }
    TermFieldMatchDataPosition * getFixed() { return reinterpret_cast<TermFieldMatchDataPosition *>(_data._position); }
#pragma GCC diagnostic pop
    const TermFieldMatchDataPosition * getMultiple() const { return _data._positions._positions; }
    TermFieldMatchDataPosition * getMultiple() { return _data._positions._positions; }
    int32_t  getElementWeight() const { return empty() ? 1 : allocated() ? getMultiple()->getElementWeight() : getFixed()->getElementWeight(); }
    uint32_t getMaxElementLength() const { return empty() ? 0 : allocated() ? _data._positions._maxElementLength : getFixed()->getElementLen(); }
    void appendPositionToAllocatedVector(const TermFieldMatchDataPosition &pos);
    void allocateVector();
    void resizePositionVector(size_t sz) __attribute__((noinline));

    enum { FIELDID_MASK = 0x1fff};

    uint32_t  _docId;
    // 3 upper bits used to tell if it is use for RawScore, SinglePos or multiPos.
    uint16_t  _fieldId;
    uint16_t  _sz;
    Features  _data;

    friend class ::MatchDataHeapTest;

public:
    PositionsIterator begin() const { return allocated() ? getMultiple() : getFixed(); }
    PositionsIterator end() const { return allocated() ? getMultiple() + _sz : empty() ? getFixed() : getFixed()+1; }
    size_t size() const { return _sz; }
    size_t capacity() const { return allocated() ? _data._positions._allocated : 1; }
    void reservePositions(size_t sz) {
        if (sz > capacity()) {
            if (!allocated()) {
                allocateVector();
                if (sz <= capacity()) return;
            }
            resizePositionVector(sz);
        }
    }

    /**
     * Create empty object. To complete object setup, field id must be
     * set.
     **/
    TermFieldMatchData();
    TermFieldMatchData(const TermFieldMatchData & rhs);
    ~TermFieldMatchData();
    TermFieldMatchData & operator = (const TermFieldMatchData & rhs);

    /**
     * Swaps the content of this object with the content of the given
     * term field match data object.
     *
     * @param rhs The object to swap with.
     **/
    void swap(TermFieldMatchData &rhs);

    MutablePositionsIterator populate_fixed() {
        assert(!allocated());
        if (_sz == 0) {
            new (_data._position) TermFieldMatchDataPosition();
            _sz = 1;
        }
        return getFixed();
    }

    /**
     * Set which field this object has match information for.
     *
     * @return this object (for chaining)
     * @param fieldId field id
     **/
    TermFieldMatchData &setFieldId(uint32_t fieldId) {
        if (fieldId == IllegalFieldId) {
            fieldId = FIELDID_MASK;
        } else {
            assert(fieldId < FIELDID_MASK);
        }
        _fieldId = (_fieldId & ~FIELDID_MASK) | fieldId;
        return *this;
    }

    /**
     * Obtain the field id
     *
     * @return field id
     **/
    uint32_t getFieldId() const {
        return __builtin_expect((_fieldId & FIELDID_MASK) != FIELDID_MASK, true) ? (_fieldId & FIELDID_MASK) : IllegalFieldId;
    }

    /**
     * Reset the content of this match data and prepare it for use
     * with the given docid.
     *
     * @return this object (for chaining)
     * @param docId id of the document we are generating match information for
     **/
    TermFieldMatchData &reset(uint32_t docId) {
        _docId = docId;
        _sz = 0;
        if (isRawScore()) {
            _data._rawScore = 0.0;
        } else if (isMultiPos()) {
            _data._positions._maxElementLength = 0;
        }
        return *this;
    }

    /**
     * Reset only the docid of this match data and prepare it for use
     * with the given docid. Assume all other are not touched.
     *
     * @return this object (for chaining)
     * @param docId id of the document we are generating match information for
     **/
    TermFieldMatchData &resetOnlyDocId(uint32_t docId) {
        _docId = docId;
        return *this;
    }

    /**
     * Indicate a match for a given docid and inject a raw score
     * instead of detailed match data. The raw score can be picked up
     * in the ranking framework by using the rawScore feature for the
     * appropriate field.
     *
     * @return this object (for chaining)
     * @param docId id of the document we have matched
     * @param score a raw score for the matched document
     **/
    TermFieldMatchData &setRawScore(uint32_t docId, feature_t score) {
        resetOnlyDocId(docId);
        enableRawScore();
        _data._rawScore = score;
        return *this;
    }
    TermFieldMatchData & enableRawScore() {
        _fieldId = _fieldId | 0x8000;
        return *this;
    }

    /**
     * Obtain the raw score for this match data.
     *
     * @return raw score
     **/
    feature_t getRawScore() const {
        return __builtin_expect(isRawScore(), true) ? _data._rawScore : 0.0;
    }

    void setSubqueries(uint32_t docId, uint64_t subqueries) {
        resetOnlyDocId(docId);
        _data._subqueries = subqueries;
    }

    uint64_t getSubqueries() const {
        if (!empty() || isRawScore()) {
            return 0;
        }
        return _data._subqueries;
    }

    /**
     * Obtain the document id for which the data contained in this object is valid.
     *
     * @return document id
     **/
    uint32_t getDocId() const {
        return _docId;
    }

    /**
     * Obtain the weight of the first occurrence in this field, or 1
     * if no occurrences are present. This function is intended for
     * attribute matching calculations.
     *
     * @return weight
     **/
    int32_t getWeight() const {
        if (__builtin_expect(_sz == 0, false)) {
            return 1;
        }
        return __builtin_expect(allocated(), false) ? getMultiple()->getElementWeight() : getFixed()->getElementWeight();
    }

    /**
     * Add occurrence information to this match data for the current
     * document.
     *
     * @return this object (for chaining)
     * @param pos low-level occurrence information
     **/
    TermFieldMatchData &appendPosition(const TermFieldMatchDataPosition &pos) {
        if (_sz == 0 && !allocated()) {
            _sz = 1;
            new (_data._position) TermFieldMatchDataPosition(pos);
        } else {
            if (!allocated()) {
                allocateVector();
            }
            appendPositionToAllocatedVector(pos);
        }
        return *this;
    }

    /**
     * Obtain an object that gives access to the low-level occurrence
     * information stored in this object.
     *
     * @return field position iterator
     **/
    FieldPositionsIterator getIterator() const {
        const uint32_t len(getMaxElementLength());
        return FieldPositionsIterator(len != 0 ? len : FieldPositionsIterator::UNKNOWN_LENGTH, begin(), end());
    }

    /**
     * This indicates if this instance is actually used for ranking or not.
     * @return true if it is not needed.
     */
    bool  isNotNeeded() const { return _fieldId & 0x2000; }

    /**
     * Tag that this instance is not really used for ranking.
     */
    void tagAsNotNeeded() {
        _fieldId = _fieldId | 0x2000;
    }

    /**
     * Tag that this instance is used for ranking.
     */
    void tagAsNeeded() {
        _fieldId = _fieldId & ~0x2000;
    }

    /**
     * Special docId value indicating that no data has been saved yet.
     * This should match (or be above) endId() in search::queryeval::SearchIterator.
     *
     * @return constant
     **/
    static uint32_t invalidId() { return 0xdeadbeefU; }
} __attribute__((packed));

}
