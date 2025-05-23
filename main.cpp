#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <queue>
#include <cmath>
#include <limits>
#include <algorithm>
#include <array>

// Define constants for better readability and maintainability
const int GRID_SIZE = 20;
const int CELL_SIZE = 25; // size of each cell in pixels
const int MARGIN = 10;    // margin around grid and panel
const int PANEL_SPACING = 10;
const float BUTTON_PADDING = 20.f;
const float TEXT_OFFSET_X = 10.f;
const float TEXT_OFFSET_Y = 5.f;
const int PANEL_WIDTH_ADDITION = 200; // Additional width for the panel

// Define costs for movement
const float CARDINAL_COST = 1.0f;
const float DIAGONAL_COST = std::sqrt(2.0f); // Approximately 1.414

// Directions for 8-directional movement (static const to avoid re-creation)
static const std::array<sf::Vector2i, 8> directions = {
    sf::Vector2i(1, 0), sf::Vector2i(0, 1), sf::Vector2i(-1, 0), sf::Vector2i(0, -1),
    sf::Vector2i(1, 1), sf::Vector2i(-1, 1), sf::Vector2i(1, -1), sf::Vector2i(-1, -1)};

// Struct to store animation steps with direct colors
struct AnimationStep
{
    sf::Vector2i coord;
    sf::Color color; // The color this cell should become at this step
};

int main()
{
    const unsigned windowWidth = static_cast<unsigned>(GRID_SIZE * CELL_SIZE + PANEL_WIDTH_ADDITION);
    const unsigned windowHeight = static_cast<unsigned>(GRID_SIZE * CELL_SIZE + 2 * MARGIN);

    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "Grid Pathfinding Visualizer");
    window.setFramerateLimit(60);

    // Load font for button text and messages
    sf::Font font;
    if (!font.openFromFile("arial.ttf"))
    {
        // Failed to load font
        return -1;
    }

    // Grid and wall data
    std::vector<std::vector<bool>> wall(GRID_SIZE, std::vector<bool>(GRID_SIZE, false));
    // Grid state will directly store colors for animation
    std::vector<std::vector<sf::Color>> gridColors(GRID_SIZE, std::vector<sf::Color>(GRID_SIZE));

    // Start and end positions
    int startX = 0, startY = 0;
    int endX = GRID_SIZE - 1, endY = GRID_SIZE - 1;

    // Animation data
    std::vector<AnimationStep> dijkstraAnimationSteps;
    std::vector<AnimationStep> astarAnimationSteps;
    int currentDijkstraAnimFrame = -1; // -1 means not animating
    int currentAstarAnimFrame = -1;
    sf::Clock animationClock;
    sf::Time animationDelay = sf::milliseconds(20); // Adjust for faster/slower animation

    // Message display for pathfinding results
    sf::Text messageText(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(sf::Vector2f(static_cast<float>(GRID_SIZE * CELL_SIZE + MARGIN), static_cast<float>(windowHeight - 50)));
    std::string currentMessage = "";

    // Prepare button text
    sf::Text dijkstraText(font);
    dijkstraText.setString("DIJKSTRA");
    dijkstraText.setFillColor(sf::Color::White);
    dijkstraText.setCharacterSize(20);

    sf::Text aText(font);
    aText.setString("A*");
    aText.setFillColor(sf::Color::White);
    aText.setCharacterSize(20);

    // Compute button sizes based on text bounds (using SFML 3.0 sf::Rect<T> access)
    auto diBounds = dijkstraText.getLocalBounds();
    auto aBounds = aText.getLocalBounds();
    float buttonWidth = std::max(diBounds.size.x, aBounds.size.x) + BUTTON_PADDING;
    float diButtonHeight = diBounds.size.y + BUTTON_PADDING;
    float aButtonHeight = aBounds.size.y + BUTTON_PADDING;

    // Create button shapes
    sf::RectangleShape diButton(sf::Vector2f(buttonWidth, diButtonHeight));
    diButton.setFillColor(sf::Color::Green);
    sf::RectangleShape aButton(sf::Vector2f(buttonWidth, aButtonHeight));
    aButton.setFillColor(sf::Color(255, 0, 255)); // magenta

    // Position panel and buttons
    float panelX = static_cast<float>(GRID_SIZE * CELL_SIZE + MARGIN);
    float panelY = static_cast<float>(MARGIN);
    diButton.setPosition(sf::Vector2f(panelX, panelY));
    aButton.setPosition(sf::Vector2f(panelX, panelY + diButtonHeight + PANEL_SPACING));

    // Position text inside buttons
    dijkstraText.setPosition(sf::Vector2f(panelX + TEXT_OFFSET_X, panelY + TEXT_OFFSET_Y));
    aText.setPosition(sf::Vector2f(panelX + TEXT_OFFSET_X, panelY + diButtonHeight + PANEL_SPACING + TEXT_OFFSET_Y));

    // Function to reset grid colors for animation
    auto resetGridColors = [&]()
    {
        for (int r = 0; r < GRID_SIZE; ++r)
        {
            for (int c = 0; c < GRID_SIZE; ++c)
            {
                if (wall[r][c])
                {
                    gridColors[r][c] = sf::Color::White; // Walls are white
                }
                else
                {
                    gridColors[r][c] = sf::Color(255, 200, 0); // Unexplored traversable cells are orange
                }
            }
        }
        // Start and End nodes are always blue and override other colors
        gridColors[startY][startX] = sf::Color::Blue;
        gridColors[endY][endX] = sf::Color::Blue;
    };

    resetGridColors(); // Initial setup of grid colors

    while (window.isOpen())
    {
        // Event handling (SFML 3.0 style using std::optional and type-safe access)
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (auto *key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
            else if (auto *mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    int mx = mouse->position.x;
                    int my = mouse->position.y;

                    // Grid area click: toggle wall
                    if (mx >= 0 && mx < GRID_SIZE * CELL_SIZE && my >= 0 && my < GRID_SIZE * CELL_SIZE)
                    {
                        int col = mx / CELL_SIZE;
                        int row = my / CELL_SIZE;
                        // Prevent toggling start/end
                        if (!((col == startX && row == startY) || (col == endX && row == endY)))
                        {
                            wall[row][col] = !wall[row][col];
                        }
                        // Clear any paths, messages, and stop animations after grid change
                        dijkstraAnimationSteps.clear();
                        astarAnimationSteps.clear();
                        currentDijkstraAnimFrame = -1;
                        currentAstarAnimFrame = -1;
                        currentMessage = "";
                        resetGridColors(); // Reset visual grid
                    }
                    // Dijkstra button area click
                    else if (mx >= panelX && mx < panelX + buttonWidth &&
                             my >= panelY && my < panelY + diButtonHeight)
                    {
                        // Stop other animation and clear paths/messages
                        currentAstarAnimFrame = -1;
                        dijkstraAnimationSteps.clear();
                        astarAnimationSteps.clear(); // Clear A* steps as well
                        currentMessage = "";
                        resetGridColors(); // Reset visual grid for new animation

                        const int N = GRID_SIZE;
                        std::vector<std::vector<float>> dist(N, std::vector<float>(N, std::numeric_limits<float>::max()));
                        std::vector<std::vector<sf::Vector2i>> prev(N, std::vector<sf::Vector2i>(N, sf::Vector2i(-1, -1)));

                        struct Node
                        {
                            float d;
                            int x, y;
                        };
                        struct Cmp
                        {
                            bool operator()(Node const &a, Node const &b) const { return a.d > b.d; }
                        };
                        std::priority_queue<Node, std::vector<Node>, Cmp> pq;

                        dist[startY][startX] = 0.0f;
                        pq.push({0.0f, startX, startY});
                        dijkstraAnimationSteps.push_back({sf::Vector2i(startX, startY), sf::Color::Cyan}); // Start node is initially 'open'

                        while (!pq.empty())
                        {
                            Node node = pq.top();
                            pq.pop();
                            int cx = node.x, cy = node.y;
                            float cd = node.d;

                            // Using a small epsilon for float comparison to account for precision loss
                            if (cd > dist[cy][cx] + std::numeric_limits<float>::epsilon())
                                continue; // Already found a shorter path

                            // Mark as visited (grey), unless it's the start/end node
                            if (!((cx == startX && cy == startY) || (cx == endX && cy == endY)))
                            {
                                dijkstraAnimationSteps.push_back({sf::Vector2i(cx, cy), sf::Color(100, 100, 100)});
                            }

                            if (cx == endX && cy == endY)
                                break; // Goal reached

                            for (auto &dir : directions)
                            {
                                int nx = cx + dir.x;
                                int ny = cy + dir.y;
                                if (nx >= 0 && nx < N && ny >= 0 && ny < N && !wall[ny][nx])
                                {
                                    float moveCost = (dir.x != 0 && dir.y != 0) ? DIAGONAL_COST : CARDINAL_COST; // Calculate cost based on movement type
                                    float nd = cd + moveCost;
                                    if (nd < dist[ny][nx])
                                    {
                                        dist[ny][nx] = nd;
                                        prev[ny][nx] = sf::Vector2i(cx, cy);
                                        pq.push({nd, nx, ny});
                                        // Mark as open (cyan), unless it's the start/end node
                                        if (!((nx == startX && ny == startY) || (nx == endX && ny == endY)))
                                        {
                                            dijkstraAnimationSteps.push_back({sf::Vector2i(nx, ny), sf::Color::Cyan});
                                        }
                                    }
                                }
                            }
                        }
                        // Reconstruct Dijkstra path and add to animation steps
                        std::vector<sf::Vector2i> finalPath; // Temporary vector for path reconstruction
                        int tx = endX, ty = endY;
                        if (dist[ty][tx] < std::numeric_limits<float>::max())
                        {
                            while (!(tx == startX && ty == startY))
                            {
                                finalPath.emplace_back(tx, ty);
                                sf::Vector2i p = prev[ty][tx];
                                tx = p.x;
                                ty = p.y;
                            }
                            finalPath.emplace_back(startX, startY);
                            std::reverse(finalPath.begin(), finalPath.end()); // Reverse to get start-to-end

                            // Add path steps to animation after all search steps
                            for (const auto &p : finalPath)
                            {
                                if (!((p.x == startX && p.y == startY) || (p.x == endX && p.y == endY)))
                                {
                                    dijkstraAnimationSteps.push_back({p, sf::Color::Green}); // Path nodes are green
                                }
                            }
                        }
                        else
                        {
                            currentMessage = "Dijkstra: No Path Found!";
                        }
                        currentDijkstraAnimFrame = 0; // Start animation
                        animationClock.restart();
                    }
                    // A* button area click
                    else if (mx >= panelX && mx < panelX + buttonWidth &&
                             my >= panelY + diButtonHeight + PANEL_SPACING &&
                             my < panelY + diButtonHeight + PANEL_SPACING + aButtonHeight)
                    {
                        // Stop other animation and clear paths/messages
                        currentDijkstraAnimFrame = -1;
                        astarAnimationSteps.clear();
                        dijkstraAnimationSteps.clear(); // Clear Dijkstra steps as well
                        currentMessage = "";
                        resetGridColors(); // Reset visual grid for new animation

                        const int N = GRID_SIZE;
                        std::vector<std::vector<float>> g_cost(N, std::vector<float>(N, std::numeric_limits<float>::max()));
                        std::vector<std::vector<sf::Vector2i>> prev(N, std::vector<sf::Vector2i>(N, sf::Vector2i(-1, -1)));

                        struct Node
                        {
                            float f, g; // f_cost and g_cost
                            int x, y;
                        };
                        struct Cmp2
                        {
                            bool operator()(Node const &a, Node const &b) const { return a.f > b.f; }
                        };
                        std::priority_queue<Node, std::vector<Node>, Cmp2> pq;

                        auto heuristic = [&](int x, int y)
                        {
                            int dx = std::abs(x - endX);
                            int dy = std::abs(y - endY);
                            return static_cast<float>(std::max(dx, dy)); // Chebyshev distance for 8-directional movement
                        };

                        g_cost[startY][startX] = 0.0f;
                        pq.push({heuristic(startX, startY), 0.0f, startX, startY});
                        astarAnimationSteps.push_back({sf::Vector2i(startX, startY), sf::Color::Cyan}); // Start node is initially 'open'

                        while (!pq.empty())
                        {
                            Node node = pq.top();
                            pq.pop();
                            int cx = node.x, cy = node.y;
                            float cg = node.g;

                            // Using a small epsilon for float comparison to account for precision loss
                            if (cg > g_cost[cy][cx] + std::numeric_limits<float>::epsilon())
                                continue; // Already found a shorter path

                            // Mark as visited (grey), unless it's the start/end node
                            if (!((cx == startX && cy == startY) || (cx == endX && cy == endY)))
                            {
                                astarAnimationSteps.push_back({sf::Vector2i(cx, cy), sf::Color(100, 100, 100)});
                            }

                            if (cx == endX && cy == endY)
                                break; // Goal reached

                            for (auto &dir : directions)
                            {
                                int nx = cx + dir.x;
                                int ny = cy + dir.y;
                                if (nx >= 0 && nx < N && ny >= 0 && ny < N && !wall[ny][nx])
                                {
                                    float moveCost = (dir.x != 0 && dir.y != 0) ? DIAGONAL_COST : CARDINAL_COST; // Calculate cost based on movement type
                                    float ng = cg + moveCost;
                                    if (ng < g_cost[ny][nx])
                                    {
                                        g_cost[ny][nx] = ng;
                                        prev[ny][nx] = sf::Vector2i(cx, cy);
                                        float f = ng + heuristic(nx, ny);
                                        pq.push({f, ng, nx, ny});
                                        // Mark as open (cyan), unless it's the start/end node
                                        if (!((nx == startX && ny == startY) || (nx == endX && ny == endY)))
                                        {
                                            astarAnimationSteps.push_back({sf::Vector2i(nx, ny), sf::Color::Cyan});
                                        }
                                    }
                                }
                            }
                        }
                        // Reconstruct A* path and add to animation steps
                        std::vector<sf::Vector2i> finalPath; // Temporary vector for path reconstruction
                        int tx = endX, ty = endY;
                        if (g_cost[ty][tx] < std::numeric_limits<float>::max())
                        {
                            while (!(tx == startX && ty == startY))
                            {
                                finalPath.emplace_back(tx, ty);
                                sf::Vector2i p = prev[ty][tx];
                                tx = p.x;
                                ty = p.y;
                            }
                            finalPath.emplace_back(startX, startY);
                            std::reverse(finalPath.begin(), finalPath.end()); // Reverse to get start-to-end

                            // Add path steps to animation after all search steps
                            for (const auto &p : finalPath)
                            {
                                if (!((p.x == startX && p.y == startY) || (p.x == endX && p.y == endY)))
                                {
                                    astarAnimationSteps.push_back({p, sf::Color(255, 0, 255)}); // Path nodes are magenta
                                }
                            }
                        }
                        else
                        {
                            currentMessage = "A*: No Path Found!";
                        }
                        currentAstarAnimFrame = 0; // Start animation
                        animationClock.restart();
                    }
                }
            }
        }

        // Update animation frame for Dijkstra
        if (currentDijkstraAnimFrame != -1 && animationClock.getElapsedTime() >= animationDelay)
        {
            if (currentDijkstraAnimFrame < dijkstraAnimationSteps.size())
            {
                const auto &step = dijkstraAnimationSteps[currentDijkstraAnimFrame];
                // Only update if it's not the start/end node, which should always remain blue
                if (!((step.coord.x == startX && step.coord.y == startY) || (step.coord.x == endX && step.coord.y == endY)))
                {
                    gridColors[step.coord.y][step.coord.x] = step.color;
                }
                currentDijkstraAnimFrame++;
            }
            else
            {
                currentDijkstraAnimFrame = -1; // Animation finished
            }
            animationClock.restart();
        }

        // Update animation frame for A*
        if (currentAstarAnimFrame != -1 && animationClock.getElapsedTime() >= animationDelay)
        {
            if (currentAstarAnimFrame < astarAnimationSteps.size())
            {
                const auto &step = astarAnimationSteps[currentAstarAnimFrame];
                // Only update if it's not the start/end node, which should always remain blue
                if (!((step.coord.x == startX && step.coord.y == startY) || (step.coord.x == endX && step.coord.y == endY)))
                {
                    gridColors[step.coord.y][step.coord.x] = step.color;
                }
                currentAstarAnimFrame++;
            }
            else
            {
                currentAstarAnimFrame = -1; // Animation finished
            }
            animationClock.restart();
        }

        // Rendering
        window.clear(sf::Color::Black);

        // Draw grid cells based on their current color in gridColors
        sf::RectangleShape cellShape;
        cellShape.setOutlineThickness(1.f);
        cellShape.setOutlineColor(sf::Color::Red);
        cellShape.setSize(sf::Vector2f(static_cast<float>(CELL_SIZE), static_cast<float>(CELL_SIZE)));

        for (int r = 0; r < GRID_SIZE; ++r)
        {
            for (int c = 0; c < GRID_SIZE; ++c)
            {
                cellShape.setFillColor(gridColors[r][c]);
                cellShape.setPosition(sf::Vector2f(static_cast<float>(c * CELL_SIZE), static_cast<float>(r * CELL_SIZE)));
                window.draw(cellShape);
            }
        }

        // Ensure Start and End cells are always blue and drawn on top
        // This is important because animation steps might temporarily color them
        sf::RectangleShape startShape(sf::Vector2f(static_cast<float>(CELL_SIZE), static_cast<float>(CELL_SIZE)));
        startShape.setFillColor(sf::Color::Blue);
        startShape.setPosition(sf::Vector2f(static_cast<float>(startX * CELL_SIZE), static_cast<float>(startY * CELL_SIZE)));
        window.draw(startShape);

        sf::RectangleShape endShape(sf::Vector2f(static_cast<float>(CELL_SIZE), static_cast<float>(CELL_SIZE)));
        endShape.setFillColor(sf::Color::Blue);
        endShape.setPosition(sf::Vector2f(static_cast<float>(endX * CELL_SIZE), static_cast<float>(endY * CELL_SIZE)));
        window.draw(endShape);

        // Draw panel buttons and text
        window.draw(diButton);
        window.draw(aButton);
        window.draw(dijkstraText);
        window.draw(aText);

        // Draw message if any
        if (!currentMessage.empty())
        {
            messageText.setString(currentMessage);
            window.draw(messageText);
        }

        window.display();
    }

    return 0;
}