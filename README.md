Mining frequent closed itemsets (FCI) over a data
stream is a challenging task as dynamic processing requires a
large number of extra itemsets be maintained which may cause
an overhead in both processing and storage. Yet recent work in
the field has highlighted the need to limit resource consumption,
e.g. due to environment constraints. In a search for a better tradeoff
between storage and efficiency, we designed Ciclad, a novel
intersection-based sliding-window stream CI miner. It leverages
in-depth insights into FCI evolution upon transaction add/remove
and combines parsimonious storage and quick access techniques.
Experimental results indicate that although it stores all closed
itemsets, Ciclad’s memory consumption is much lower that its
main competitor method. Moreover, it outperforms that method
on sparse datasets and remains competitive on dense ones.