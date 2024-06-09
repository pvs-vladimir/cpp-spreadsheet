#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <set>

class Sheet;

class Cell : public CellInterface {
private:
    class Impl;
    
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
    void CheckCyclicDependencies(std::unique_ptr<Impl> tmp_impl);
    void UpdateReferences();
    void InvalidateAllCache(bool flag);

private:
    class Impl {
    public:
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;
        virtual bool HasCache() const;
        virtual void InvalidateCache();
        virtual ~Impl() = default;
    };
    
    class EmptyImpl final : public Impl {
    public:
        Value GetValue() const override;
        std::string GetText() const override;
    }; 
    
    class TextImpl final : public Impl {
    public:
        explicit TextImpl(std::string text);
        Value GetValue() const override;
        std::string GetText() const override;
    
    private:
        std::string text_;
    };
    
    class FormulaImpl final : public Impl {
    public:
        explicit FormulaImpl(std::string text, SheetInterface& sheet);
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        bool HasCache() const override;
        void InvalidateCache() override;
    private:
        std::unique_ptr<FormulaInterface> formula_;
        SheetInterface& sheet_;
        mutable std::optional<FormulaInterface::Value> cache_;
    };
    
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::set<Cell*> dependent_cells_;
    std::set<Cell*> referenced_cells_;
};