def find_peak_full(matrix):
    """
    2D Peak Finder using the FULL cross:
    The candidate window includes all cells along the submatrix's boundary (top, bottom, left, right)
    as well as all cells in the center row and center column.
    """
    rows = len(matrix)
    cols = len(matrix[0])
    return find_peak_full_helper(matrix, 0, rows - 1, 0, cols - 1)

def find_peak_full_helper(matrix, top, bottom, left, right):
    # Base case: empty submatrix
    if top > bottom or left > right:
        return None

    mid_row = (top + bottom) // 2
    mid_col = (left + right) // 2

    # Build the FULL CROSS: include the submatrix's entire top and bottom rows,
    # left and right columns, and the center row and center column.
    coords = set()
    for col in range(left, right + 1):
        coords.add((top, col))
        coords.add((bottom, col))
        coords.add((mid_row, col))
    for row in range(top, bottom + 1):
        coords.add((row, left))
        coords.add((row, right))
        coords.add((row, mid_col))

    best_coord = None
    best_val = float('-inf')
    for (r, c) in coords:
        if matrix[r][c] > best_val:
            best_val = matrix[r][c]
            best_coord = (r, c)

    r, c = best_coord
    # Check all four neighbors within the current submatrix.
    for dr, dc in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
        nr, nc = r + dr, c + dc
        if nr < top or nr > bottom or nc < left or nc > right:
            continue
        if matrix[nr][nc] > matrix[r][c]:
            # Recurse in the quadrant in the direction of the larger neighbor.
            if dr == -1:
                return find_peak_full_helper(matrix, top, mid_row - 1, left, right)
            elif dr == 1:
                return find_peak_full_helper(matrix, mid_row + 1, bottom, left, right)
            elif dc == -1:
                return find_peak_full_helper(matrix, top, bottom, left, mid_col - 1)
            elif dc == 1:
                return find_peak_full_helper(matrix, top, bottom, mid_col + 1, right)
    return best_coord

def find_peak_limited(matrix):
    """
    2D Peak Finder using the LIMITED cross:
    The candidate window consists only of the center row and center column.
    (If a center row/column cell lies on the submatrix boundary, it is naturally included.)
    This version ignores the remainder of the submatrix boundary.
    """
    rows = len(matrix)
    cols = len(matrix[0])
    return find_peak_limited_helper(matrix, 0, rows - 1, 0, cols - 1)

def find_peak_limited_helper(matrix, top, bottom, left, right):
    if top > bottom or left > right:
        return None

    mid_row = (top + bottom) // 2
    mid_col = (left + right) // 2

    # LIMITED CROSS: only include the center row and center column.
    coords = set()
    for col in range(left, right + 1):
        coords.add((mid_row, col))
    for row in range(top, bottom + 1):
        coords.add((row, mid_col))

    best_coord = None
    best_val = float('-inf')
    for (r, c) in coords:
        if matrix[r][c] > best_val:
            best_val = matrix[r][c]
            best_coord = (r, c)

    r, c = best_coord
    # Check all neighbors (from the full submatrix) for a larger value.
    for dr, dc in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
        nr, nc = r + dr, c + dc
        if nr < top or nr > bottom or nc < left or nc > right:
            continue
        if matrix[nr][nc] > matrix[r][c]:
            if dr == -1:
                return find_peak_limited_helper(matrix, top, mid_row - 1, left, right)
            elif dr == 1:
                return find_peak_limited_helper(matrix, mid_row + 1, bottom, left, right)
            elif dc == -1:
                return find_peak_limited_helper(matrix, top, bottom, left, mid_col - 1)
            elif dc == 1:
                return find_peak_limited_helper(matrix, top, bottom, mid_col + 1, right)
    return best_coord

# ============================
# Test Cases
# ============================

test_cases = [
    # Example 1: Simple 3x3 ascending matrix.
    ("Simple ascending 3x3",
     [
         [1, 2, 3],
         [4, 5, 6],
         [7, 8, 9]
     ]),

    # Example 2: Counterexample 5x5 where the left boundary has a high value.
    ("Counterexample 5x5 (high value on boundary)",
     [
         [0, 0, 0, 0, 0],
         [0, 1, 1, 1, 0],
         [10, 1, 0, 1, 0],
         [0, 1, 1, 1, 0],
         [0, 0, 0, 0, 0]
     ]),

    # Example 3: Symmetric 5x5 matrix with a clear center peak.
    ("Symmetric 5x5 with center peak",
     [
         [1, 2, 3, 2, 1],
         [2, 4, 5, 4, 2],
         [3, 5, 9, 5, 3],
         [2, 4, 5, 4, 2],
         [1, 2, 3, 2, 1]
     ]),

    # Example 4: 5x5 matrix with maximum values only on the boundary.
    ("5x5 with boundary maximum",
     [
         [10,  2,  2,  2, 10],
         [ 2,  1,  1,  1,  2],
         [ 2,  1,  0,  1,  2],
         [ 2,  1,  1,  1,  2],
         [10,  2,  2,  2, 10]
     ]),

    # Example 5: 5x5 matrix with a plateau in the center.
    ("5x5 with plateau center",
     [
         [5, 5, 5, 5, 5],
         [5, 7, 7, 7, 5],
         [5, 7, 7, 7, 5],
         [5, 7, 7, 7, 5],
         [5, 5, 5, 5, 5]
     ]),
]

# ============================
# Run and Print Test Cases
# ============================

def print_matrix(matrix):
    for row in matrix:
        print(row)
    print()

for desc, matrix in test_cases:
    print("=== Test Case:", desc, "===")
    print("Matrix:")
    print_matrix(matrix)

    peak_full = find_peak_full(matrix)
    peak_limited = find_peak_limited(matrix)

    if peak_full:
        r, c = peak_full
        print("FULL Cross Peak -> Position: {} Value: {}".format(peak_full, matrix[r][c]))
    else:
        print("FULL Cross Peak -> No peak found.")

    if peak_limited:
        r, c = peak_limited
        print("LIMITED Cross Peak -> Position: {} Value: {}".format(peak_limited, matrix[r][c]))
    else:
        print("LIMITED Cross Peak -> No peak found.")
    
    print("\n")
