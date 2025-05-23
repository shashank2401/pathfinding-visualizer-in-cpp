# Pathfinding Visualizer

A C++ application built with SFML 3.0 that interactively visualizes Dijkstra's and A\* pathfinding algorithms on a grid. Users can define obstacles (walls) and observe the step-by-step search process, culminating in the display of the geometrically accurate shortest path found by each algorithm.

---

## Features

- **Interactive Grid:** Click cells to toggle them between traversable ground (orange) and impassable walls (white).
- **Dijkstra's Algorithm:** Visualize the classic shortest-path algorithm with **geometric costs** (1 unit for straight moves, √2 for diagonals).
- **A\* Search Algorithm:** Informed search using **Octile distance heuristic** for 8-directional movement with geometric costs.
- **Animated Search:** Observe the algorithms' exploration process step-by-step:
  - **Open nodes** (considered): Cyan
  - **Visited nodes** (processed): Grey
  - **Final path**: Green (Dijkstra) / Magenta (A\*)
- **SFML 3.0 Compatibility:** Modern API usage including:
  - `std::optional` for event handling
  - `sf::Vector2<T>` for coordinates/sizes

---

## Usage

- **Toggle Walls:** Left-click grid cells (white ↔ orange)
- **Run Dijkstra:** Click green "DIJKSTRA" button (right panel)
- **Run A\*:** Click magenta "A\*" button (right panel)
- **Clear Animation:** Toggle any wall to reset visualization
- **Exit:** Esc key or close window

---

## Implementation Notes
- **8-directional movement** uses geometric costs:
  - **Straight moves**: 1.0 unit
  - **Diagonal moves**: √2 units (~1.414)
- **Octile distance heuristic** ensures optimal performance with geometric costs
- **Path costs** represent true geometric distance on the grid

---

## Author
**Shashank Raj**