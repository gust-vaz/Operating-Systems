# Cyclist Elimination Race Simulator

This project is a simulation of a cyclist elimination race based on the "Win and Out" rules, implemented in C using POSIX threads. The simulator models a velodrome race where cyclists are progressively eliminated until only one remains. Also, this is the second assignment of the Operating Systems course.

## Features

- Cyclists move concurrently on a circular track.
- Cyclists randomly change speeds between 30 km/h and 60 km/h.
- Simulates overtaking, lane shifts, and physical constraints of a velodrome.
- Cyclists may "break down" with a 15% probability every 6 laps.
- Outputs real-time and final race standings.
- Optional debug mode to display track status every 60ms.

## Requirements

- **Operating System**: GNU/Linux
- **Compiler**: GCC
- **Libraries**: `pthread`

## Compilation

To compile the program, run the following command in the directory containing the source files and the `Makefile`:

```bash
make
```

This will generate the executable `ep2`.

## Execution

The program requires two mandatory arguments and an optional flag:

```bash
./ep2 <d> <k> [-debug]
```

- `<d>`: Length of the track in meters (100 ≤ d ≤ 2500).
- `<k>`: Number of cyclists (5 ≤ k ≤ 5 × (d - 1)).
- `-debug`: Optional flag to enable debug mode, displaying the track state every 60ms.

### Examples

1. Run a race with a 500-meter track and 50 cyclists:
   ```bash
   ./ep2 500 50
   ```

2. Run the same race in debug mode:
   ```bash
   ./ep2 500 50 -debug
   ```

## Output

### Normal Mode

- Displays rankings after every lap.
- Outputs final standings at the end of the race, including:
  - Rank, lap times, and status of all cyclists.
  - Identification of cyclists who "broke down" and their respective lap.

### Debug Mode

- Displays the state of the track every 60ms.
- Outputs only the final standings at the end of the race.

## How It Works

1. **Initialization**: Cyclists are distributed on the track, with a maximum of 5 cyclists side by side per position.
2. **Movement**: Each cyclist moves independently, respecting speed limits and track constraints.
3. **Speed Adjustment**: Cyclists have a probability of changing their speed each lap:
   - 70% chance to accelerate from 30 km/h to 60 km/h.
   - 50% chance to decelerate from 60 km/h to 30 km/h.
4. **Elimination**: 
   - Every 2×n laps, the first cyclist to finish the lap is removed and ranked.
   - Cyclists who "break down" are removed from the race and noted in the results.
5. **Completion**: The race ends when only one cyclist remains.

## Debug Mode Example Output

For a 100-meter track with 20 cyclists:
```text
.   .   .   6   7   1   8   .   .   .
.   .   2   9   3   10  13  .   .   .
.   .   4   11  14  19  5   12  .   .
```

## Final Output Example

```text
============= Relatório Final =============
Ranking:
[ 1° ] Ciclista 4   (360ms)
[ 2° ] Ciclista 12  (420ms)
[ 3° ] Ciclista 8   (480ms)
...
Quebraram:
Ciclista 15 na volta [6]
Ciclista 7  na volta [12]
```