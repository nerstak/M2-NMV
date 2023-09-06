# NMV: Gestion de la mémoire virtuelle

## Exercice 1

### Question 1

On a besoin d'une fonction identité pour pouvoir écrire et modifier les tables des pages (celles-ci sont accessibles par
adresses physiques et non par adresses virtuelles).
Ainsi, pour quelques addresses (sous-ensemble des adresses des entrées des tables des pages), on sait qu'on aura un
mappage identitaire tel que l'adresse virtuelle et physique seront identiques.

Cette supposition n'est valide que pour les adresses des tables, puisque ce sont les seules adresses pour lesquelles il
est essentiel de résoudre facilement l'adresse virtuelle en physique.

### Question 2

- Pour chaque niveau de la table des pages, on a 512 entrées (2<sup>9</sup>).

- La MMU détermine qu'une entrée est valide si le bit `P` est à 1.

- La MMU détermine qu'une entrée est terminale en comptant le nombre de redirection lors de la résolution :
  
  - Si les huge pages sont activées, on s'arrête à 3 redirections
  - Si les huge pages ne sont pas activées, on s'arrête à 4 redirections

### Question 3

Voir la fonction `print_pgt()` dans `kernel/memory.c`.

## Exercice 2

### Question 1

L'index de l'entrée correspond à l'adresse virtuelle à laquelle on applique un masque (via un bitwise `&`) de `0x1FF`.
Le masque est appliqué sur les bits correspondants au niveau actuel, tel que :

```c
int indexLvl = (vaddr & (0x1FF << (12 + 9 * (lvl - 1)))) >> (12 + 9 * (lvl - 1));
```

### Question 2

Voir la fonction `map_page()` dans `kernel/memory.c`.

## Exercice 3

### Question 1

L'adresse `0x2000000030` est comprise entre `0x2000000000` et `0x00007fffffffffff`.
Cette zone correspond à la partie dédiée à la mémoire "User (text + data + heap)".
Or, on doit mapper explicitement cette zone, ce qui n'est pas le cas.

### Question 2

Les plages d'adresses de la table courante correspondant au kernel doivent être conservées.
En effet, il faut que les modifications en mode Kernel soient visibles par les autres processus.
On n'a besoin que d'une seule entrée PML3 (entièrement dédiée à cela) pour garder les entrées Kernel.

Pour garantir cela, la première entrée PML3 des différents processus pointent vers la même PML 2.
Cette approche nécessite cependant un processus de Lock pour éviter que des processus entrent en conflit.

### Question 3

payload_size = `ctx->load_end_paddr - ctx->load_paddr`
Adresse virtuelle du début du payload : `ctx->load_vaddr`
Adresse virtuelle de fin du payload : `ctx->load_vaddr + payload_size - 4096` (4096 => taille de page sans huge page)

Adresse virtuelle du début du BSS : `ctx->load_vaddr + payload_size`
Adresse virtuelle de fin du BSS : `bss_end_vaddr - 4096` (4096 => taille de page sans huge page)
bss_start_vaddr = `ctx->load_vaddr + payload_size`

### Question 4

Voir la fonction `load_task()` dans `kernel/memory.c`

### Question 5

Voir la fonction `set_task()` dans `kernel/memory.c`

## Exercice 4

### Question 1

Voir la fonction `mmap()` dans `kernel/memory.c`

### Question 2

L'adresse `0x1ffffffff8` est comprise entre `0x40000000` et `0x1ffffffff8`.
Cette zone correspond à la partie dédiée à la mémoire "User (stack)".

Cette accès mémoire parait légitime, puisque l'utilisateur est autorisé à mapper la zone du stack.

### Question 3

L'allocation paresseuse est une optimisation qui consiste à n'allouer la mémoire seulement lorsqu'elle est utilisée.
Cette allocation se réalise lors du premier accès.

### Question 4

Voir la fonction `pgfault()` dans `kernel/memory.c`.

## Exercice 5

### Question 1

Voir la fonction `munmap()` dans `kernel/memory.c`.

### Question 2

Dans `task/adversary.c`, à la ligne 24, on s'aperçoit que l'on fait un appel système à la fonction `munmap()`, sur
l'adresse `0x1fffff3000`.
Or cette adresse n'a à priori pas été mappée. Malgré cela, on essaye de lire la page (élément par élément) et de
vérifier que ceux-ci sont bien vides (car unmappé).
Ce comportement (munmap + accès après munmap) génère l'échec de la tâche Adversary.

### Question 3

Voir la fonction `munmap()` dans `kernel/memory.c`.