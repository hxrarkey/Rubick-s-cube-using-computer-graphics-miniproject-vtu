# 3×3×3 Rubik's Cube Simulator 🧩

This project is a **basic Rubik's Cube simulator** that allows users to **manually shuffle and solve a 3×3×3 cube**. It provides an interactive way to understand cube movements and solving techniques.

## Features

✅ **Manual Shuffling** – Users can scramble the cube manually.  
✅ **Manual Solving** – Solve the cube step by step with move tracking.  
✅ **Basic Cube Rotation** – Supports standard face rotations (**R, L, U, D, F, B**).  
✅ **Simple Visual Representation** – Displays the cube state after each move.

## Mathematical Representation

The Rubik's Cube transformations are often represented using group theory. Each face rotation can be expressed as a **permutation of elements**:

\[
F, F', B, B', R, R', L, L', U, U', D, D'
\]

Where:  
- \( F \) (Front), \( B \) (Back), \( R \) (Right), \( L \) (Left), \( U \) (Up), \( D \) (Down)  
- \( ' \) denotes a counterclockwise turn (e.g., \( F' \) is **front face counterclockwise**)  

A solved cube state can be represented using identity permutations:

\[
I = E
\]

Where **\( E \)** is the identity transformation (no movement).

