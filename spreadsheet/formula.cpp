#include "formula.h"

#include "FormulaAST.h"

#include <sstream>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {};

        Value Evaluate(const SheetInterface& sheet) const override {
            Value res;
            try {
                res = ast_.Execute(sheet);
            }
            catch (const FormulaError& err) {
                res = err;
            }
            return res;
        }

        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            const auto& pos_list = ast_.GetCells();
            std::set<Position> s(pos_list.begin(), pos_list.end());
            return { s.begin(), s.end() };
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(expression);
    }
    catch (...) {
        throw FormulaException("Parsing formula from expression was failure"s);
    }
}