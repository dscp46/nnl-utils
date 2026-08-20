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

Let $`\gamma(n)`$  the path from the root to node $`n`$.
```math
\pi(n) = b_1 \dots b_p
```
```math
\sigma(n) = b_{p+1} \dots b_{d_n}
```
```math
\gamma(n) \in \{0,1\}^k, \quad \gamma(n) = \pi(n) \Vert \sigma(n) \Vert 1 \Vert \underbrace{0 \dots 0}_{k - d_n - 1} = b_1 \dots b_{d_n} 1 \underbrace{0 \dots 0}_{k - d_n - 1}, \quad \forall n \vert d_T(n,r) < k-1 
```
## Position of a node's state in a tree state table

Let $`\xi(n)`$ the position of a node's state within a tree state table.
```math
$$ \xi(n) = \underbrace{\left(2^{d-p+1} - 1\right)}_{\text{size of a subtree}} \cdot \underbrace{\lfloor \frac{\gamma(n)}{2^{k-d_T(n,r_p)}} \rfloor }_{\pi(n)} + \underbrace{\left(2^{d_T(n,r_p)}-1\right)}_{\text{levels above us in }T_p} + \underbrace{\left(\lfloor\frac{\gamma}{2^{k-d_T(n,r)}}\rfloor \mod \left(2^{d_T(n,r)-d_T(n,r_p)}\right)\right)}_{\sigma(n)}, \quad \xi(n) \in \left[ 0 , \sum_{Tp \in F_p(T)} \vert V(T_p) \vert \right[ $$
```
