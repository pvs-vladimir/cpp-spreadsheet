#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetRegularCell(Position pos) const;
    Cell* GetRegularCell(Position pos);

private:
    std::unordered_map<Position, std::unique_ptr<Cell>, Position::Hash> cells_;
    
    void IsPositionValid(Position pos, const std::string& error_message) const;
};