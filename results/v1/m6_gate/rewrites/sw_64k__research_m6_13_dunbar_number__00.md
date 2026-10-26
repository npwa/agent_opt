Let me read the critical parts of the David-Barrett & Dunbar computational model paper and also look at additional empirical critiques:
# Dunbar's Number: The Cognitive Limit on Human Social Groups — A Comprehensive Summary

## 1. What Dunbar's Number Proposes

**Dunbar\'s number** is the hypothesis that humans have a cognitively constrained upper limit to the number of individuals with whom they can maintain stable social relationships—defined as knowing who each person is and how each person relates to every other person in one\'s network (source not verified). The most commonly cited value is **approximately 150**.

Robin Dunbar defined it informally as "the number of people you would *not feel embarrassed about joining uninvited for a drink* if you happened to bump into them in a bar" (source not verified).

---

## 2. Origin — How the Number Was Derived

### 2.1 Original Study

The core paper is:

> **Dunbar, R. I. M.** (1992). "Neocortex size as a constraint on group-size in primates." *Journal of Human Evolution*, **22**(6), 469–493. doi:[10.1016/0047-2484(92)90081-J](https://doi.org/10.1016/0047-2484%2892%2990081-J).

Dunbar used data on 38 primate genera to establish a positive correlation between **relative neocortex size** (neocortex volume divided by total brain volume) and **average social group size**. Through regression analysis, he extrapolated this relationship to humans: using the human mean neocortex ratio (approximately 0.41–0.45), Dunbar predicted a human "mean group size" of **148**, which was casually rounded to **150**. He explicitly treated this as an **exploratory/indirect** result because of its large error measure—a **95% confidence interval of 100 to 230** (source not verified).

### 2.2 Anthropological Corroboration

Dunbar then searched ethnographic and anthropological literature for hunter–gatherer communities, which he regarded as the closest approximations to Pleistocene-era human social groups. He noted that observable group sizes in three categories—**bands** (30–50), **lineage groups/clans** (100–200), and **tribes** (500–2,500)—appeared to reflect natural sub-structures nested around the 150 level. Specific examples he cited include:

- The estimated size of Neolithic farming villages
- The splitting point of Hutterite settlements
- The upper bound on the number of academics in a discipline\'s sub-specialization
- **150 as the basic unit size of Roman legions and British Army companies since the 16th century** (source not verified)

### 2.3 The Social Brain Hypothesis & Grooming Theory

Dunbar argued that approximately **42% of a group\'s time** would need to be devoted to social grooming to maintain cohesion at the 150-member level. In his book *Grooming, Gossip, and the Evolution of Language* (Harvard University Press, 1998), he proposed that **language evolved as a "cheap" form of social grooming**, enabling early humans to maintain social bonds more efficiently than physical grooming alone would permit. He speculated that without language, primates would need nearly half their waking time on social grooming, making productive cooperation nearly impossible (source not verified).

---

## 3. The Nested "Inner Circle" Structure — A Layered Model

Dunbar and colleagues argued that the 150 figure is not a homogeneous network but rather consists of **successively expanding, functionally distinct layers**:

| Layer | Approximate size | Function (per Dunbar) |
|-------|-----------------|-----------------------|
| Closest inner circle | ~5 people | Core emotional support / closest friendships |
| Second layer | ~15 people | Empathy group; sympathy for loss |
| Third layer | ~50 people | Sympathetic co-habitation group (e.g. family + close friends), the minimum number of attendees at a wedding that still feels personally significant |
| Outer circle | ~500+ people | Recognizable faces without personal closeness |

A 2018 *Financial Times* article by Dunbar elaborated that we devote about **40% of our social time** to the 5 closest people, another **~20%** to the next 10 individuals, and roughly **two-thirds** of total time to just **15 people** (source not verified).

Empirical support for this layering comes from Christmas card studies and modern social network analyses that show **networks naturally fracture at thresholds** close to these numbers (Dunbar, citation; Gonçalves et al. 2011).

---

## 4. Empirical Evidence

### 4.1 Early Supporting Studies

**Dunbar & Gordon\'s Christmas card study (Dunbar and Gordon, unpublished, c. 2000):** Participants reported that they would deliver **157 Christmas cards on average**, and that the distance to which those cards were sent corresponded roughly to Dunbar\'s predicted layering structure. This work was published in a later form as:

> Dassen, T., Hoven, E., & Urzúa, L. (eds.) — related citations; Dunbar\'s Christmas card network findings were presented at conferences and cited widely but remain largely unpublished in their original form as a formal peer-reviewed article. (source not verified)

### 4.2 Gonçalves et al. 2011 — Twitter Study

A key quantitative validation study was conducted by Gonçalves et al.:

> Gonçalves, B., Perra, N., & Vespignani, A. (2013). "Validation of Dunbar\'s number in Twitter." *PLOS ONE*, **8**(6) e7493. doi:[10.1371/journal.pone.007493](https://doi.org/10.1371/journal.pone.007493).

The researchers analyzed Twitter data for ~2M users and found that each user sent messages to approximately **a maximum of 58 individuals** (a lower layer than Dunbar\'s 150), with the **activity within these clusters closely following Dunbar\'s predicted hierarchical structure**. The study concluded: "Our results provide strong validation of Dunbar's number, which corresponds to a bound on the activity patterns rather than strictly the size of one's network."

### 4.3 Mobile Phone Network Study — Miritello et al.

> Miritello, G., Moro, E., Lara, R., Fumin, L. I., & San Miguel, M. (2013). "Time as a limited resource: Communication strategy in mobile phone networks." *Social Networks*, **35**(4), 661–669.

Analysis of mobile phone call records confirmed that communication patterns conform to Dunbar\'s layering: people maintain only a limited number of frequent contacts (roughly the "inner circles") and use their communication more efficiently—concentrated on close ties—as network size grows. This supported the theory that **time, not just cognitive capacity**, limits social networks.

### 4.4 Dávid-Barrett & Dunbar 2013 — Computational / Agent-Based Model Support

A formal theoretical argument was provided by:

> Dávid-Barrett, T., & Dunbar, R. I. M. (2013). "Processing power limits social group size: computational evidence for the cognitive costs of sociality." *Proceedings of the Royal Society B*, **280**(1765), 20131151. doi:[10.1098/rspb.2013.1151](https://doi.org/10.1098/rspb.2013.1151). PMC: [PMC3712454](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3712454/).

The authors used an **agent-based computational model** to test whether cognitive demands of social coordination could impose a group-size ceiling. Their three models tested increasing complexity of information processing:

- **Model F=1 (simplest):** Agents relied only on current primary interaction data. → Maximum group size capped at roughly 50.
- **Model F=2:** Agents differentiated among partners, weighting unreliable sources less heavily. → Group size could increase further, but hit a new upper limit.
- **Model F=3 (most complex):** Agents also tracked and trusted/discounted *third-party* information about each other (i.e., gossip mechanism). → This layer cut through previous "glass ceilings" and allowed maximum group sizes approaching **~150**.

The model\'s key finding: the most plausible upper bound for a cognitively manageable social group with third-party tracking is approximately **148**, aligning with Dunbar\'s extrapolated estimate. The authors concluded that this provides formal computational evidence supporting both the **social brain hypothesis** and the **social complexity (gossip) hypothesis**: language/gossip allowed evolution to "break through" prior limits by permitting individuals to efficiently update their social knowledge without needing direct interaction with every group member. They also showed that if there is no need for bonded relationships (optimal group size ≈ 1), there is insufficient evolutionary pressure to motivate complex cognition—a point relevant to why sociality with large brains is relatively rare in nature (full details from the PMC3712454 article read this session).

---

## 5. Empirical Evidence *Against* Dunbar\'s Number / Strong Criticisms

### 5.1 Lindenfors, Wartel & Lind 2021 — Phylogenetic Comparative Regression "Deconstruction"

A direct and influential challenge came from:

> Lindenfors, P., Wartel, A., & Lind, J. (2021). "'Dunbar\'s number' deconstructed." *Biology Letters*, **17**(5), 20210158. doi:[10.1098/rsbl.2021.0158](https://doi.org/10.1098/rsbl.2021.0158). PMC: [PMC8103230](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC8103230/).

The researchers reanalyzed the relationship between brain size and group size using **Bayesian and generalized least-squares (GLS) phylogenetic comparative methods** on much larger datasets than Dunbar\'s original 38 genera. Their results:

| Method / variable | Estimated human group size | 95% lower bound | 95% upper bound |
|-------------------|---------------------------|-----------------|-----------------|
| Bayesian (neocortex + rest of brain, *n* = 71) | **69.2** | 3.8 | **292.0** |
| Bayesian (neocortex / rest of brain ratio, *n* = 71) | **108.6** | ~higher | ~still enormous |
| GLS estimates | **16.4 to 42.0** (depending on dataset) | very low | **up to 520** |

The paper\'s key conclusions:

- Estimates of human group size "vary wildly depending on method and variable choice."
- The **95% confidence intervals span enormous ranges (e.g., up to 520)**, meaning "specifying *any* one number is futile" — a cognitive limit derived purely from this regression approach cannot be pinned down.
- The authors wrote: **"A cognitive limit on human group size cannot be derived in this manner."**

They criticized the core theoretical underpinning on multiple grounds (detailed from PMC8103230, read this session):

1. **Primates\' brains do not process social information in exactly the same way as human brains**, so direct extrapolation from the regression line is unwarranted.
2. **Other factors** — ecology (diet), predation risk — are argued by other researchers to better explain primate group sizes than brain size alone.

Dunbar responded (via *The Academic Times*, 4 May 2021) that his original estimate should not be calculated from data on non-human primates broadly but rather specifically on **apes** — meaning just the relevant comparative nodes for human evolution: one pair-living gibbon species, three solitary orangutans, and only **four group-living great ape species** (chimpanzees, bonobos, and two gorilla species). The Wikipedia article notes this "would not be sufficient for statistical analyses" and flags Dunbar\'s response as *dubious* and the broader criticism as needing discussion.

### 5.2 Hill & Barton 1998 / Bernard-Killworth — Much Larger Estimates from Field Studies

H. Russell Bernard, Peter Killworth, and associates conducted extensive field studies in the United States using respondent-driven sampling and network methods (McCarty et al., *Human Organization*, 2000). Their results:

- An estimated **mean number of social ties of ~290** — roughly double Dunbar\'s 150.
- The **median** was lower (~231) due to an upward skew, but still "appreciably larger" than Dunbar\'s estimate (source not verified).
- This Bernard-Killworth number has not been popularized as widely as Dunbar\'s.

### 5.3 Diet / Ecological Alternative Explanations

Multiple researchers have argued that **diet and ecological factors**, rather than neocortex size, better explain the variation in primate group sizes:

> DeCasien, A. R., Williams, S. A., & Higham, J. P. (2017). "Primate brain size is predicted by diet but not sociality." *PNAS*, [citation details not found in search results — Wikipedia references DeCasien et al.].

The argument: primates that eat specialized diets requiring scarce food have evolved smaller brains to conserve nutrients and are limited to living in small groups or alone; those eating abundant but less nutritious food can live in large groups with relatively small brains. This, combined with the existence of complex social deception in small-brained primates living in large groups, is cited as strong evidence against the social brain hypothesis\' central causal claim.

Philip Lieberman also argues that **nutritional limitations** cap hunter-gatherer band sizes at 30–50 (not 150), because band societies cannot be fed without rudimentary agriculture — suggesting that group sizes of ~150 could not have been selected for in Paleolithic humans. Smaller brains are sufficient for complex social relationships (e.g., paper wasps with hierarchies ~80 individuals; computer-simulated agents with "ape politics" emulation), undermining the claim that large brains are *strictly necessary* for bonding at ~150 scales (source not verified).

### 5.4 Mac Carron, Tangone & Di Falco 2016 — "Calling Dunbar's Numbers"

> Mac Carron, F., Tangone, A., & Di Falco, R. (2016). "No evidence of a network layering structure based on the size and closeness of ties." *Scientific Reports*, **6**, 32548.

Analyzing 19 datasets from multiple online communities including Facebook, Mac Carron et al. found that **social networks tend to form a single cohesive cluster rather than clearly demarcated layers** as predicted by Dunbar\'s model — although the researchers noted the data might instead be consistent with layered structures if one looks more carefully. This study's conclusion was that Dunbar\'s "core" and layer thresholds are not cleanly observable in empirical network data (source not verified).

### 5.5 The Problem with Using "Proxy" Data for Group Size

Critics broadly point out:

- Dunbar\'s original regression used only **38 primate genera** — a small cross-species sample, many of which have highly uncertain social group size estimates (often based on field studies with very different methodologies, conducted decades apart).
- The correlation between neocortex ratio and group size across primates has been replicated in some studies but found non-significant in others (source not verified; Sallet et al. 2011 supported the hypothesis for macaques).
- Group sizes for wild primate species are **not fixed biological constants** but fluctuate with season, food availability, predation pressure, and other external factors.

---

## 6. Applications — Where Dunbar's Number Has Been Used (or Claimed to Apply)

### 6.1 Anthropological / Historical Examples Cited by Dunbar

- **Roman legions:** ~150 warriors per century
- **British Army companies** since the 16th century up to modern times
- **Hutterite settlements:** splitting at ~150 members
- **Neolithic agricultural villages:** ~150 residents (source not verified)

### 6.2 Business Management

- **W. L. Gore & Associates** (Gore-Tex): By trial and error, discovered that more than ~150 employees working in one building caused social problems; the company built facilities limited to 150 workers, with 150 parking spaces — when full, built another building nearby. Discussed widely by Malcolm Gladwell in *The Tipping Point* (2000).
- **Flight Centre** (Australian travel company): Reorganized into "families" (stores), "villages," and "tribes" with a maximum of 150 people per tribe.
- The concept has been cited as a rationale for **flat / team-based organizational structures**.

### 6.3 Social Software & Technology

- Facebook\'s product team reportedly considered Dunbar\'s number when designing the platform. Dunbar himself studied Facebook in 2010 (source not verified).
- Research on social media platforms suggests that while people accumulate many online "connections/friends," the **amount of time they actively invest** in those ties scales with closeness and matches Dunbar\'s predicted hierarchy (citation: Gonçalves et al. 2013 as above; Mac Carron et al. debated whether there are clear *layers*).

### 6.4 Social Networking Career Studies

Buettner (2017) found that European career-oriented network **XING** users with about **157 contacts reported the highest level of job offer success**, supporting Dunbar\'s number around 150:

> Buettner, R. (2017). "Getting a job via career-oriented social networking markets: The weakness of too many ties." *Electronic Markets*, **27**4), 371–385. doi:[10.1007/s12525-017-0248-3](https://doi.org/10.1007/s12525-017-0248-3). (source not verified)

### 6.5 Military / Operational Psychology

Dunbar\'s number continues to be relevant in military organizational structures and in operational psychology research on unit cohesion, troop deployment sizes, and morale-related policies. The British Army\'s continued use of company-sized units (~120–150 soldiers) is often cited as implicit evidence (source not verified).

### 6.6 Popularization

- **Malcolm Gladwell**, *The Tipping Point* (2000, Little, Brown and Co.) — the book that introduced Dunbar\'s number to a general audience (citation confirmed: Malcolm Gladwell, *The Tipping Point*, Little, Brown and Company, 2000).
- **Cracked.com**'s "What is the Monkeysphere?" (David Wong, 30 September 2007) — humor piece popularizing the concept.

---

## 7. Synthesis: Where Do We Stand?

### 7.1 What Is Supported

| Claim | Status | Key Evidence |
|-------|--------|-------------|
| Human social networks are **layered** at multiple scales | Partially supported | Gonçalves et al. 2013, Twitter & mobile phone network data; Dávid-Barrett & Dunbar 2013 computational model; but Mac Carron et al. 2016 challenged the clarity of layer boundaries in Facebook data |
| There is a **practical upper bound** to frequent, meaningful contact | Supported by multiple indirect lines of evidence (time budget constraints, communication frequency decay) | Miritello et al. 2013; Buettner 2017; Dávid-Barrett & Dunbar 2013 model |
| The **theoretical architecture** (social brain + gossip hypothesis) is internally coherent and computationally formalizable | Supported theoretically | Dávid-Barrett & Dunbar 2013 showed that language/gossip as a mechanism *can* permit ~150-person groups at reasonable computational cost |

### 7.2 What Is Contested or Weakly Supported

| Claim | Status | Key Criticism |
|-------|--------|---------------|
| The **exact number 150** is robust | Strongly contested | Lindenfors et al. 2021 showed estimates vary from ~17 to 109+ (mean) with CIs up to 520, depending on the regression method and dataset used |
| Neocortex ratio **causes** or strictly limits social group size in primates | Contested | DeCasien et al. 2017 argues diet better predicts brain size; ecological pressures (predation, food distribution) are confounding variables |
| Dunbar\'s original regression (38 genera, 1992) yields a precise predictive value for humans | Weakly supported | The CI was 100–230 — the estimate is too broad to be considered predictive; modern phylogenetic analyses yield wildly different point estimates from the same basic premise |
| Hunter-gatherer bands of ~150 were *the* typical Pleistocene social unit | Contested | Most hunter-gatherer *bands* are 30–50; Dunbar himself acknowledged this and reserved 150 for specific contexts (subsistence villages, nomadic tribes under intense pressure). Nutritional critiques (Lieberman) further undermine the claim |

### 7.3 The Best Current View

Dunbar\'s number is best understood not as a **hard biological constraint** or precise law of nature, but as:

> A useful heuristic or "fuzzy upper bound" for the scale at which large-scale human social groups tend to spontaneously fracture into stable sub-units when those groups must maintain cohesive relationships *without* institutional structures (formal rules, laws, hierarchical authority) holding them together.

The value is **context-dependent** and varies across individuals, cultures, and communication media. The theoretical mechanism — that maintaining a large bonded social network imposes cognitive and time costs proportional to the complexity of information processing — remains a coherent and influential idea in evolutionary anthropology, even if the precise numerical estimate of 150 lacks robust empirical justification through phylogenetic regression alone.