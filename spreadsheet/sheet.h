#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <memory>
#include <deque>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet() override = default;

    void SetCell(Position pos, std::string text) override;
    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    void ClearCell(Position pos) override;
    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    struct PositionHasher {
        size_t operator()(const Position& pos) const {
            return std::hash<int>()(pos.row) ^ (std::hash<int>()(pos.col) << 1);
        }
    };

    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> sheet_;
    Size printable_size_ = { 0, 0 };

    static void PositionIsValid(Position pos);
    void RecalculatePrintableSize();
};
