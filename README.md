# Pathfinding Visualizer

A C++ application built with SFML 3.0 that interactively visualizes Dijkstra's and A* pathfinding algorithms on a grid. Users can define obstacles (walls) and observe the step-by-step search process, culminating in the display of the shortest path found by each algorithm.

---

## Features

- **Interactive Grid:** Click cells to toggle them between traversable ground and impassable walls.
- **Dijkstra's Algorithm:** Visualize the classic shortest-path algorithm with uniform movement costs.
- **A* Search Algorithm:** Informed search using Chebyshev distance heuristic for 8-directional movement with uniform cost (1 unit per step).
- **Animated Search:** Observe the algorithms' exploration process step-by-step:
  - **Open nodes** (considered): Cyan
  - **Visited nodes** (processed): Grey
  - **Final path**: Green (Dijkstra) / Magenta (A*)
- **SFML 3.0 Compatibility:** Modern API usage including:
  - `std::optional` for event handling
  - `sf::Vector2<T>` for coordinates/sizes

---

## Usage

- **Toggle Walls:** Left-click grid cells (white ↔ orange)
- **Run Dijkstra:** Click green "DIJKSTRA" button (right panel)
- **Run A*:** Click magenta "A*" button (right panel)
- **Clear Animation:** Toggle any wall to reset visualization
- **Exit:** Esc key or close window

---

## Implementation Notes
- **8-directional movement** uses uniform cost (1 unit) for simplicity and visual clarity
- **Chebyshev heuristic** ensures optimal performance with uniform costs
- **Path costs** represent step count rather than geometric distance

---

## Author
**Shashank Raj**