#pragma once

#include <vector>
#include <QColor>
#include <algorithm>
#include <cmath>
class MaterialColorManager {
private:
    std::vector<QColor> allColors;
    std::vector<QColor> availableColors;
    QColor lastColor;
    bool hasLastColor = false;
    int totalUsedCount = 0; // Track total colors used across all cycles

    // Helper function to check if two QColors are equal
    bool colorsEqual(const QColor& c1, const QColor& c2) const {
        return c1.red() == c2.red() && c1.green() == c2.green() && c1.blue() == c2.blue();
    }

    // Calculate color distance using Euclidean distance in RGB space
    double colorDistance(const QColor& c1, const QColor& c2) const {
        double dr = c1.red() - c2.red();
        double dg = c1.green() - c2.green();
        double db = c1.blue() - c2.blue();
        return std::sqrt(dr * dr + dg * dg + db * db);
    }

    // Get color family/hue category for better differentiation
    int getColorFamily(const QColor& color) const {
        int r = color.red();
        int g = color.green();
        int b = color.blue();

        // Determine dominant color component and relationships
        if (r >= g && r >= b) {
            if (g >= b) {
                if (r - g < 50) return 0; // Yellow family (high R, high G)
                else return 1;            // Red family (high R, low G)
            }
            else {
                return 2; // Magenta family (high R, low G, some B)
            }
        }
        else if (g >= r && g >= b) {
            if (r >= b) {
                return 0; // Yellow-Green family
            }
            else {
                if (b > 100) return 3; // Cyan family (high G, high B)
                else return 4;         // Green family (high G, low B)
            }
        }
        else { // b is highest
            if (r >= g) {
                return 2; // Blue-Magenta family
            }
            else {
                if (g > 100) return 3; // Blue-Cyan family
                else return 5;         // Blue family
            }
        }
    }

    // Get brightness level of color
    double getBrightness(const QColor& color) const {
        return (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue());
    }

    // Check if colors are too similar
    bool isTooSimilar(const QColor& c1, const QColor& c2) const {
        // Check color family
        if (getColorFamily(c1) == getColorFamily(c2)) {
            // Same family, check distance
            return colorDistance(c1, c2) < 100.0;
        }

        // Different families, check if they're still too close in RGB space
        return colorDistance(c1, c2) < 60.0;
    }

    // Remove color from available colors vector
    void removeColorFromAvailable(const QColor& color) {
        auto it = std::find_if(availableColors.begin(), availableColors.end(),
            [this, &color](const QColor& c) { return colorsEqual(c, color); });
        if (it != availableColors.end()) {
            availableColors.erase(it);
        }
    }

public:
    MaterialColorManager() {
        initializeMaterialColors();
        resetAvailableColors();
    }

    void initializeMaterialColors() {
        allColors = {
            // Red family - warm colors
            QColor(244, 67, 54),   // Red 500
            QColor(229, 57, 53),   // Red 600

            // Pink family
            QColor(233, 30, 99),   // Pink 500
            QColor(216, 27, 96),   // Pink 600

            // Purple family
            QColor(156, 39, 176),  // Purple 500
            QColor(142, 36, 170),  // Purple 600

            // Deep Purple family
            QColor(103, 58, 183),  // Deep Purple 500
            QColor(94, 53, 177),   // Deep Purple 600

            // Indigo family - cool blues
            QColor(63, 81, 181),   // Indigo 500
            QColor(57, 73, 171),   // Indigo 600

            // Blue family
            QColor(33, 150, 243),  // Blue 500
            QColor(30, 136, 229),  // Blue 600

            // Light Blue family
            QColor(3, 169, 244),   // Light Blue 500
            QColor(3, 155, 229),   // Light Blue 600

            // Lime family - yellow-green
            QColor(205, 220, 57),  // Lime 500
            QColor(192, 202, 51),  // Lime 600

            // Teal family
            QColor(0, 150, 136),   // Teal 500
            QColor(0, 137, 123),   // Teal 600

            // Green family - pure greens
            QColor(76, 175, 80),   // Green 500
            QColor(67, 160, 71),   // Green 600

            // Light Green family
            QColor(139, 195, 74),  // Light Green 500
            QColor(124, 179, 66),  // Light Green 600


            // Yellow family - bright yellows
            QColor(255, 235, 59),  // Yellow 500
            QColor(251, 192, 45),  // Yellow 600

            // Amber family
            QColor(255, 193, 7),   // Amber 500
            QColor(255, 179, 0),   // Amber 600

            // Orange family - warm oranges
            QColor(255, 152, 0),   // Orange 500
            QColor(251, 140, 0),   // Orange 600

            // Deep Orange family
            QColor(255, 87, 34),   // Deep Orange 500
            QColor(244, 81, 30),   // Deep Orange 600

            // Brown family - earth tones
            QColor(121, 85, 72),   // Brown 500
            QColor(109, 76, 65),   // Brown 600

            // Grey family - neutrals
            QColor(158, 158, 158), // Grey 500
            QColor(117, 117, 117), // Grey 600

            // Blue Grey family - cool neutrals
            QColor(96, 125, 139),  // Blue Grey 500
            QColor(84, 110, 122)   // Blue Grey 600
        };
    }

    void resetAvailableColors() {
        availableColors.clear();
        availableColors = allColors; // Copy all colors to available
    }

    // Get next differentiated color with automatic cycling
    QColor getNextDifferentiatedColor() {
        // If all colors are used, reset the cycle
        if (availableColors.empty()) {
            resetAvailableColors();
            std::cout << "--- Completed full color cycle, starting new cycle ---\n";
        }

        QColor selectedColor;
        bool colorFound = false;

        if (hasLastColor && availableColors.size() > 1) {
            // Find the most differentiated color
            double maxScore = -1;

            for (const auto& color : availableColors) {
                if (isTooSimilar(color, lastColor)) {
                    continue; // Skip similar colors
                }

                // Calculate differentiation score
                double distance = colorDistance(color, lastColor);
                int familyDiff = (getColorFamily(color) != getColorFamily(lastColor)) ? 1 : 0;
                double brightnessDiff = std::abs(getBrightness(color) - getBrightness(lastColor));

                // Combined score: distance + family difference + brightness difference
                double score = distance + (familyDiff * 100) + (brightnessDiff * 0.5);

                if (score > maxScore) {
                    maxScore = score;
                    selectedColor = color;
                    colorFound = true;
                }
            }

            // If no non-similar color found, pick the most distant one
            if (!colorFound) {
                double maxDistance = 0;
                for (const auto& color : availableColors) {
                    double distance = colorDistance(color, lastColor);
                    if (distance > maxDistance) {
                        maxDistance = distance;
                        selectedColor = color;
                        colorFound = true;
                    }
                }
            }
        }

        // Fallback: pick first available color
        if (!colorFound && !availableColors.empty()) {
            selectedColor = availableColors[0];
        }

        // Remove selected color from available vector
        removeColorFromAvailable(selectedColor);
        lastColor = selectedColor;
        hasLastColor = true;
        totalUsedCount++;

        return selectedColor;
    }

    // Get information about current state
    size_t getAvailableCount() const { return availableColors.size(); }
    size_t getTotalColors() const { return allColors.size(); }
    int getTotalUsedCount() const { return totalUsedCount; }
    int getCurrentCycle() const { return (totalUsedCount - 1) / allColors.size() + 1; }
    int getPositionInCycle() const { return ((totalUsedCount - 1) % allColors.size()) + 1; }

    // Reset everything
    void reset() {
        resetAvailableColors();
        hasLastColor = false;
        totalUsedCount = 0;
    }

    // Print color information
    void printColorInfo(const QColor& color) const {
        std::cout << "RGB(" << color.red() << ", "
            << color.green() << ", "
            << color.blue() << ")";
    }

    void printStatus() const {
        std::cout << "Cycle: " << getCurrentCycle()
            << ", Position: " << getPositionInCycle()
            << "/" << getTotalColors()
            << ", Available: " << getAvailableCount();
    }

    // Get string representation of color family for debugging
    std::string getColorFamilyName(const QColor& color) const {
        int family = getColorFamily(color);
        switch (family) {
        case 0: return "Yellow";
        case 1: return "Red";
        case 2: return "Magenta";
        case 3: return "Cyan";
        case 4: return "Green";
        case 5: return "Blue";
        default: return "Unknown";
        }
    }
};
