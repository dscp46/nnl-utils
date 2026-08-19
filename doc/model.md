# Tree Model

## Conventions
In the following document, the operator $` \Vert `$ is defined as the binary-string concatenation.

## Base definitions
Let $`T`$ be a binary tree of depth $`d`$.

Let 
```math
\mathcal{F}_p(T) = \{ T_n \mid n \in V(T), d_T(n,r) = p \}
```
where $`T_n`$ is the subtree rooted at $`n`$.

Then
```math
\left\vert \mathcal{F}_p(T) \right\vert = 2^p
```
and, for a perfect binary tree of total depth $` d `$
```math
depth(T_n) = d-p
```

Consequently, the total number of nodes across the partitioned subtrees is
```math
\sum_{T_n \in \mathcal{F}_p(T)} \left\vert V(T_n) \right\vert = 2^{p} \cdot (2^{d-p+1}-1)
```

## Path of a node in the tree

Let $`\pi(n)`$  the partition prefix of a node $`n`$ of depth $`d_n`$ 

Let $`\sigma(n)`$ the partition index of a node $`n`$ of depth $`d_n`$ 

Let $`path(n)`$  the path from the root to node $`n`$

$$ \pi(n) = b_1 \dots b_p $$
$$\sigma(n) = b_{p+1} \dots b_{d_n} $$
$$ path(n) = \pi(n) \Vert \sigma(n) = b_1 \dots b_{d_n} $$
