#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    IsPositionValid(pos, "Failed to set a cell: invalid position"s);

    const auto& cell = cells_.find(pos);
    if (cell == cells_.end()) {
        cells_.emplace(pos, std::make_unique<Cell>(*this));
    }

    cells_.at(pos)->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetRegularCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetRegularCell(pos);
}

void Sheet::ClearCell(Position pos) {
    IsPositionValid(pos, "Failed to clear a cell: invalid position"s);

    const auto& cell = cells_.find(pos);
    if (cell != cells_.end() && cell->second != nullptr) {
        cell->second->Clear();
        if (!cell->second->IsReferenced()) {
            cell->second.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size result;
    
    for (auto it = cells_.begin(); it != cells_.end(); ++it) {
        if (it->second != nullptr) {
            const int col = it->first.col;
            const int row = it->first.row;
            result.rows = std::max(result.rows, row + 1);
            result.cols = std::max(result.cols, col + 1);
        }
    }

    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            const auto& it = cells_.find({row, col});
            if (it != cells_.end() && it->second != nullptr && !it->second->GetText().empty()) {
                std::visit([&output](const auto value) { output << value; }, it->second->GetValue());
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            const auto& it = cells_.find({row, col});
            if (it != cells_.end() && it->second != nullptr && !it->second->GetText().empty()) {
                output << it->second->GetText();
            }
        }
        output << "\n";
    }
}

const Cell* Sheet::GetRegularCell(Position pos) const {
    IsPositionValid(pos, "Failed to get a cell: invalid position"s);

    const auto cell = cells_.find(pos);
    if (cell == cells_.end()) {
        return nullptr;
    }

    return cells_.at(pos).get();
}

Cell* Sheet::GetRegularCell(Position pos) {
    return const_cast<Cell*>(static_cast<const Sheet&>(*this).GetRegularCell(pos));
}

void Sheet::IsPositionValid(Position pos, const std::string& error_message) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException(error_message);
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
