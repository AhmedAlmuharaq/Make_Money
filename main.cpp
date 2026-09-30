#include <iostream>
#include <vector>
#include <chrono>
#include <climits>
#include <set>
#include <algorithm>
#include <iomanip>
#include <random>
#include <fstream>
#include <numeric>

using namespace std;
using namespace std::chrono;

class MakeChangeSolver {
private:
    vector<int> coins;
    vector<int> bestSolution;
    int bestCoinCount;
    long long solutionsFound;
    long long recursiveCalls = 0;
    ostream* solutionOutput = nullptr;

public:
    MakeChangeSolver(vector<int> c) : coins(c) {
        // Sort coins in descending order by default
        sort(coins.rbegin(), coins.rend());
    }

    // Helper function to print the solution in a readable format
    void printSolution(const vector<int>& counts) {
        int totalCoins = 0;
        for (size_t i = 0; i < coins.size(); ++i) {
            if (counts[i] > 0) {
                cout << counts[i] << " coins of " << fixed << setprecision(2) << coins[i] / 100.0 << " EUR\n";
                totalCoins += counts[i];
            }
        }
        cout << "Total coins used: " << totalCoins << "\n------------------------\n";
    }

    // ---------------------------------------------------------
    // a) Greedy Make Money Algorithm (Ordered vs Not Ordered)
    // ---------------------------------------------------------
    void solveA_Greedy(int amount, bool ordered) {
        cout << "=== Task a: Greedy Algorithm (Ordered = " << (ordered ? "Yes" : "No") << ") ===\n";
        vector<int> currentCoins = coins;

        if (!ordered) {
            // Fixed seed so the comparison can be repeated.
            mt19937 g(42);
            shuffle(currentCoins.begin(), currentCoins.end(), g);
        }

        cout << "Coin order (cents): ";
        for (int coin : currentCoins) cout << coin << ' ';
        cout << '\n';

        vector<int> counts(currentCoins.size(), 0);
        int remaining = amount;

        // Start measuring execution time
        auto start = high_resolution_clock::now();

        for (size_t i = 0; i < currentCoins.size(); ++i) {
            if (remaining >= currentCoins[i]) {
                counts[i] = remaining / currentCoins[i];
                remaining %= currentCoins[i];
            }
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);

        if (remaining == 0) {
            cout << "Solution found in " << duration.count() << " microseconds.\n";
            int totalCoins = 0;
            for (size_t i = 0; i < currentCoins.size(); ++i) {
                if (counts[i] > 0) {
                    cout << counts[i] << " coins of " << currentCoins[i] / 100.0 << " EUR\n";
                    totalCoins += counts[i];
                }
            }
            cout << "Total coins used: " << totalCoins << '\n';
        } else {
            cout << "No exact solution found with the current order. Remaining: " << remaining / 100.0 << " EUR\n";
        }
        cout << "------------------------\n";
    }

    // Count one greedy scan per denomination. Both input orders scan all n coins.
    void benchmarkTaskA() {
        const vector<int> sizes = {10, 25, 50, 100, 200, 400, 800};
        ofstream csv("task_a_complexity.csv");
        ofstream svg("task_a_curve.svg");
        if (!csv || !svg) {
            cerr << "Could not create Task a curve files in the current folder.\n";
            return;
        }

        csv << "denominations,ordered_scans,unordered_scans\n";
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"700\" height=\"500\" viewBox=\"0 0 700 500\">\n"
               "<rect width=\"700\" height=\"500\" fill=\"white\"/>\n"
               "<text x=\"350\" y=\"35\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"20\">Task a: greedy scan complexity</text>\n"
               "<path d=\"M 80 420 H 650 M 80 420 V 65\" fill=\"none\" stroke=\"black\" stroke-width=\"2\"/>\n"
               "<text x=\"365\" y=\"475\" text-anchor=\"middle\" font-family=\"sans-serif\">Number of denominations (n)</text>\n"
               "<text x=\"23\" y=\"250\" transform=\"rotate(-90 23 250)\" text-anchor=\"middle\" font-family=\"sans-serif\">Number of greedy scans</text>\n";
        for (int tick = 0; tick <= 800; tick += 200) {
            int x = 80 + tick * 570 / 800;
            int y = 420 - tick * 350 / 800;
            svg << "<text x=\"" << x << "\" y=\"440\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"12\">" << tick << "</text>\n";
            svg << "<text x=\"67\" y=\"" << y + 4 << "\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"12\">" << tick << "</text>\n";
        }
        svg << "<polyline points=\"";
        for (int n : sizes) {
            vector<int> testCoins;
            for (int value = n; value >= 1; --value) testCoins.push_back(value);
            int orderedScans = 0, remaining = 1235;
            for (int coin : testCoins) {
                ++orderedScans;
                remaining %= coin;
            }
            const int orderedRemainder = remaining;
            mt19937 generator(42);
            shuffle(testCoins.begin(), testCoins.end(), generator);
            int unorderedScans = 0;
            remaining = 1235;
            for (int coin : testCoins) {
                ++unorderedScans;
                remaining %= coin;
            }
            if (orderedRemainder != 0 || remaining != 0) {
                cerr << "Unexpected greedy benchmark remainder.\n";
            }
            csv << n << ',' << orderedScans << ',' << unorderedScans << '\n';
            svg << 80 + n * 570 / 800 << ',' << 420 - orderedScans * 350 / 800 << ' ';
        }
        svg << "\" fill=\"none\" stroke=\"#1769aa\" stroke-width=\"3\"/>\n"
               "<text x=\"410\" y=\"90\" font-family=\"sans-serif\" fill=\"#1769aa\">Ordered and unordered: n scans</text>\n"
               "</svg>\n";
        cout << "Task a curve saved: task_a_complexity.csv and task_a_curve.svg (both orders scan n denominations).\n";
    }

    // ---------------------------------------------------------
    // b) Recursive enumeration (generates all solutions)
    // ---------------------------------------------------------
    void solveB_RecursiveAll(int target, int index, vector<int>& currentCounts) {
        ++recursiveCalls;
        if (target == 0) {
            solutionsFound++;
            if (solutionOutput) {
                for (size_t i = 0; i < currentCounts.size(); ++i) {
                    if (i) *solutionOutput << ',';
                    *solutionOutput << currentCounts[i];
                }
                *solutionOutput << '\n';
            }
            return;
        }
        if (index >= static_cast<int>(coins.size())) return;

        int max_coins = target / coins[index];
        for (int i = max_coins; i >= 0; --i) {
            currentCounts[index] = i;
            solveB_RecursiveAll(target - i * coins[index], index + 1, currentCounts);
        }
        currentCounts[index] = 0; // Backtrack
    }

    void startTaskB(int amount) {
        cout << "=== Task b: Recursive (All Solutions) ===\n";
        vector<int> currentCounts(coins.size(), 0);
        solutionsFound = 0;
        recursiveCalls = 0;
        ofstream output("task_b_solutions.csv");
        if (!output) {
            cerr << "Could not create task_b_solutions.csv.\n";
            return;
        }
        for (size_t i = 0; i < coins.size(); ++i) {
            if (i) output << ',';
            output << "coin_" << coins[i] << "_cents";
        }
        output << '\n';
        solutionOutput = &output;

        auto start = high_resolution_clock::now();
        solveB_RecursiveAll(amount, 0, currentCounts);
        auto stop = high_resolution_clock::now();
        solutionOutput = nullptr;
        output.close();

        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Total solutions found: " << solutionsFound << "\n";
        cout << "Recursive calls: " << recursiveCalls << "\n";
        cout << "All solutions saved in task_b_solutions.csv\n";
        cout << "Enumeration and file-writing time: " << duration.count() << " microseconds.\n";
        cout << "------------------------\n";
    }

    void benchmarkTaskB() {
        const vector<int> amounts = {50, 100, 200, 400, 800, 1235};
        vector<long long> calls;
        ofstream csv("task_b_complexity.csv");
        ofstream svg("task_b_curve.svg");
        if (!csv || !svg) {
            cerr << "Could not create Task b curve files in the current folder.\n";
            return;
        }
        csv << "amount_cents,input_bits,solutions,recursive_calls,time_microseconds\n";
        for (int amount : amounts) {
            vector<int> counts(coins.size(), 0);
            solutionsFound = 0;
            recursiveCalls = 0;
            auto start = high_resolution_clock::now();
            solveB_RecursiveAll(amount, 0, counts);
            auto stop = high_resolution_clock::now();
            int bits = 0;
            for (int value = amount; value > 0; value /= 2) ++bits;
            csv << amount << ',' << bits << ',' << solutionsFound << ',' << recursiveCalls << ','
                << duration_cast<microseconds>(stop - start).count() << '\n';
            calls.push_back(recursiveCalls);
        }

        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"700\" height=\"500\" viewBox=\"0 0 700 500\">\n"
               "<rect width=\"700\" height=\"500\" fill=\"white\"/>\n"
               "<text x=\"350\" y=\"35\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"20\">Task b: recursive search growth</text>\n"
               "<path d=\"M 80 420 H 650 M 80 420 V 65\" fill=\"none\" stroke=\"black\" stroke-width=\"2\"/>\n"
               "<text x=\"365\" y=\"475\" text-anchor=\"middle\" font-family=\"sans-serif\">Input bits for target amount</text>\n"
               "<text x=\"23\" y=\"250\" transform=\"rotate(-90 23 250)\" text-anchor=\"middle\" font-family=\"sans-serif\">Recursive calls</text>\n";
        const long long maximum = calls.back();
        svg << "<polyline points=\"";
        for (size_t i = 0; i < amounts.size(); ++i) {
            const int x = 80 + static_cast<int>(i) * 570 / 5;
            const int y = 420 - static_cast<int>(calls[i] * 350 / maximum);
            svg << x << ',' << y << ' ';
        }
        svg << "\" fill=\"none\" stroke=\"#a0352b\" stroke-width=\"3\"/>\n";
        for (size_t i = 0; i < amounts.size(); ++i) {
            const int x = 80 + static_cast<int>(i) * 570 / 5;
            const int y = 420 - static_cast<int>(calls[i] * 350 / maximum);
            svg << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"4\" fill=\"#a0352b\"/>\n";
            svg << "<text x=\"" << x << "\" y=\"445\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"11\">" << i + 6 << "</text>\n";
        }
        svg << "<text x=\"67\" y=\"74\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"11\">" << maximum << "</text>\n</svg>\n";
        cout << "Task b curve saved: task_b_complexity.csv and task_b_curve.svg\n";
    }

    // Explicit stack: same descending DFS as the recursive algorithm, without recursion.
    void startTaskB_Iterative(int amount) {
        cout << "=== Task b: Iterative enumeration ===\n";
        ofstream output("task_b_iterative_solutions.csv");
        if (!output) {
            cerr << "Could not create task_b_iterative_solutions.csv.\n";
            return;
        }
        for (size_t i = 0; i < coins.size(); ++i) {
            if (i) output << ',';
            output << "coin_" << coins[i] << "_cents";
        }
        output << '\n';
        struct Frame { int index; int remaining; int next; };
        vector<Frame> stack = {{0, amount, amount / coins[0]}};
        vector<int> counts(coins.size(), 0);
        long long found = 0;
        auto start = high_resolution_clock::now();
        while (!stack.empty()) {
            Frame& frame = stack.back();
            if (frame.next < 0) {
                counts[frame.index] = 0;
                stack.pop_back();
                continue;
            }
            const int index = frame.index;
            const int take = frame.next--;
            const int remaining = frame.remaining - take * coins[index];
            counts[index] = take;
            if (remaining == 0) {
                ++found;
                for (size_t i = 0; i < counts.size(); ++i) {
                    if (i) output << ',';
                    output << counts[i];
                }
                output << '\n';
            } else if (index + 1 < static_cast<int>(coins.size())) {
                stack.push_back({index + 1, remaining, remaining / coins[index + 1]});
            }
        }
        auto stop = high_resolution_clock::now();
        output.close();
        cout << "Iterative solutions: " << found << "; file: task_b_iterative_solutions.csv\n";
        cout << "Enumeration and file-writing time: "
             << duration_cast<microseconds>(stop - start).count() << " microseconds.\n";
        cout << "------------------------\n";
    }

    // Task c: ascending enumeration exposes successive improvements.
    struct SearchStats {
        long long calls = 0;
        long long cuts = 0;
        long long improvements = 0;
    };

    void searchImproving(int remaining, int index, vector<int>& counts, int used,
                         bool cut, SearchStats& stats, ostream& improvements) {
        ++stats.calls;
        if (remaining == 0) {
            if (used < bestCoinCount) {
                bestCoinCount = used;
                bestSolution = counts;
                ++stats.improvements;
                improvements << used;
                for (int count : counts) improvements << ',' << count;
                improvements << '\n';
            }
            return;
        }
        if (index == static_cast<int>(coins.size())) return;
        // Even using only the largest available coin needs at least this many.
        const int lowerBound = (remaining + coins[index] - 1) / coins[index];
        if (cut && bestCoinCount != INT_MAX && used + lowerBound >= bestCoinCount) {
            ++stats.cuts;
            return;
        }
        for (int take = 0; take <= remaining / coins[index]; ++take) {
            counts[index] = take;
            searchImproving(remaining - take * coins[index], index + 1,
                            counts, used + take, cut, stats, improvements);
        }
        counts[index] = 0;
    }

    void startTaskC(int amount) {
        cout << "=== Task c: Improving solutions, with and without cut ===\n";
        for (bool cut : {false, true}) {
            const string file = cut ? "task_c_improvements_with_cut.csv"
                                    : "task_c_improvements_no_cut.csv";
            ofstream output(file);
            if (!output) { cerr << "Could not create " << file << '\n'; return; }
            output << "total_coins";
            for (int coin : coins) output << ",coin_" << coin << "_cents";
            output << '\n';
            bestCoinCount = INT_MAX;
            bestSolution.clear();
            vector<int> counts(coins.size(), 0);
            SearchStats stats;
            auto start = high_resolution_clock::now();
            searchImproving(amount, 0, counts, 0, cut, stats, output);
            auto stop = high_resolution_clock::now();
            output.close();
            cout << (cut ? "With cut" : "Without cut") << ": best=";
            if (bestCoinCount == INT_MAX) cout << "none";
            else cout << bestCoinCount << " coins";
            cout << ", calls=" << stats.calls << ", pruned=" << stats.cuts
                 << ", improvements=" << stats.improvements
                 << ", time=" << duration_cast<microseconds>(stop - start).count()
                 << " microseconds; " << file << '\n';
            if (cut && bestCoinCount != INT_MAX) printSolution(bestSolution);
        }
        cout << "------------------------\n";
    }

    // Tasks d/e/f: finite, heterogeneous stock; exact branch-and-bound.
    // minDistance=1 excludes exact repeats; larger values favor diversity.
    static int distance(const vector<int>& a, const vector<int>& b) {
        int result = 0;
        for (size_t i = 0; i < a.size(); ++i) result += abs(a[i] - b[i]);
        return result;
    }

    void searchLimited(int remaining, int index, vector<int>& counts, int used,
                       const vector<int>& limits, const vector<vector<int>>& diverseFrom,
                       int minDistance, const set<vector<int>>& tabu,
                       vector<int>& best, int& bestCost, SearchStats& stats) {
        ++stats.calls;
        if (remaining == 0) {
            if (tabu.count(counts)) return;
            for (const auto& previous : diverseFrom)
                if (distance(counts, previous) < minDistance) return;
            if (used < bestCost) { bestCost = used; best = counts; }
            return;
        }
        if (index == static_cast<int>(coins.size())) return;
        const int lowerBound = (remaining + coins[index] - 1) / coins[index];
        if (bestCost != INT_MAX && used + lowerBound >= bestCost) {
            ++stats.cuts;
            return;
        }
        const int maximum = min(remaining / coins[index], limits[index]);
        for (int take = maximum; take >= 0; --take) {
            counts[index] = take;
            searchLimited(remaining - take * coins[index], index + 1,
                          counts, used + take, limits, diverseFrom, minDistance,
                          tabu, best, bestCost, stats);
        }
        counts[index] = 0;
    }

    struct LimitedResult {
        int cost = INT_MAX;
        vector<int> counts;
        SearchStats stats;
        bool found() const { return cost != INT_MAX; }
    };

    LimitedResult solveLimited(int amount, const vector<int>& limits,
                               const vector<vector<int>>& diverseFrom = {},
                               int minDistance = 1,
                               const set<vector<int>>& tabu = {}) {
        LimitedResult result;
        if (limits.size() != coins.size() || amount < 0 || minDistance < 1 ||
            any_of(limits.begin(), limits.end(), [](int limit) { return limit < 0; })) {
            cerr << "Invalid limited-supply input.\n";
            return result;
        }
        vector<int> counts(coins.size(), 0);
        searchLimited(amount, 0, counts, 0, limits, diverseFrom, minDistance,
                      tabu, result.counts, result.cost, result.stats);
        return result;
    }

    void startTaskD(int amount, const vector<int>& limits) {
        cout << "=== Task d: Heterogeneous stock limits ===\n";
        for (size_t i = 0; i < coins.size(); ++i)
            cout << coins[i] << " cents: at most " << limits[i] << " coins\n";
        LimitedResult result = solveLimited(amount, limits);
        if (result.found()) { cout << "Optimal constrained solution:\n"; printSolution(result.counts); }
        else cout << "No feasible solution.\n";
        cout << "Search calls: " << result.stats.calls << "; cuts: " << result.stats.cuts << '\n';
        vector<int> insufficient(coins.size(), 0);
        cout << "Zero-stock test: " << (solveLimited(amount, insufficient).found()
                                      ? "unexpected solution" : "no feasible solution") << '\n';
        cout << "------------------------\n";
    }

    void startTaskE(int amount, const vector<int>& limits, int n, int minDistance) {
        cout << "=== Task e: " << n << " diverse solutions (L1 distance >= "
             << minDistance << ") ===\n";
        vector<vector<int>> chosen;
        for (int i = 0; i < n; ++i) {
            LimitedResult result = solveLimited(amount, limits, chosen, minDistance);
            if (!result.found()) { cout << "No more solutions satisfy the distance rule.\n"; break; }
            cout << "Diverse solution " << i + 1 << ":\n";
            printSolution(result.counts);
            for (const auto& previous : chosen)
                cout << "Distance from earlier solution: " << distance(result.counts, previous) << '\n';
            chosen.push_back(result.counts);
        }
        cout << "------------------------\n";
    }

    void startTaskF(int amount, const vector<int>& limits, int n) {
        cout << "=== Task f: " << n << " successive best non-tabu solutions ===\n";
        set<vector<int>> tabu;
        for (int i = 0; i < n; ++i) {
            LimitedResult result = solveLimited(amount, limits, {}, 1, tabu);
            if (!result.found()) { cout << "No more non-tabu solutions.\n"; break; }
            cout << "Rank " << i + 1 << " (best outside tabu):\n";
            printSolution(result.counts);
            tabu.insert(result.counts);
        }
        cout << "------------------------\n";
    }
};

int main() {
    // Integer cents avoid floating-point rounding of monetary values.
    vector<int> coins = {500, 200, 100, 50, 20, 10, 5};
    const int targetAmount = 1235;
    const vector<int> stockLimits = {1, 2, 5, 10, 10, 10, 10};
    MakeChangeSolver solver(coins);

    solver.solveA_Greedy(targetAmount, true);
    solver.solveA_Greedy(targetAmount, false);
    solver.benchmarkTaskA();

    solver.startTaskB(targetAmount);
    solver.startTaskB_Iterative(targetAmount);
    solver.benchmarkTaskB();

    solver.startTaskC(targetAmount);
    solver.startTaskD(targetAmount, stockLimits);
    solver.startTaskE(targetAmount, stockLimits, 3, 4);
    solver.startTaskF(targetAmount, stockLimits, 3);
    return 0;
}
