## Project for Undergraduate Thesis

# COMPARISON BETWEEN OBJECT ORIENTED PROGRAMMING AND DATA ORIENTED DESIGN IN THE CONTEXT OF GAME ENGINE DEVELOPMENT

## Abstract

The video games industry has always been invested in finding ways to optimize the software component of the games they produce. Their interest in this has been so marked that they are known to be a driving force, alongside the motion picture special effects industry, behind innovations in hardware and software dedicated to solving a variety of problems ranging from photo realistic scene rendering, to AI, to high fidelity physics simulations. 

One of the conclusions games developers have reached through years of experience is that taking into account the actual physical characteristics of the machine a system is meant to run on leads to important performance gains. Stating this conclusion so plainly makes it seem obvious, but other industries such as web or enterprise management seem to not know it or to ignore it and instead tend to abstract away the hardware as one more of the many layer of an OOP software stack on top of which they do their actual development.

Data Oriented Design (DOD for short) is what the games industry is starting to adopt as a way of creating optimal software products instead of OOP, which is what has been their most predominant tool. This project takes interest in these paradigms and attempts to compare them on the grounds of the efficiency profiles each tends to lead to.

## Conclusions

On the data locality (or cache coherency) aspect of DOD this work is not very conclusive since a large real world project might not have been properly emulated, but the results obtained at least hint at the possibility that caring a lot about data layout in memory and the effects it has on the processor’s memory caches utilization might bring insufficient advantages in terms of efficiency and be not worth the extra complexity.

On the other hand it has been proven that existence based processing does provide important performance boosts to applications that would otherwise contain thousands of conditional branches causing the processor to flush the half executed instructions from the mis-predicted branches off of the pipeline, which can get very expensive. These performance benefits do also come at the cost of increased complexity.

## Document
https://drive.google.com/file/d/1C5OOetgIcYEep_eCVWgdSZSA6p4kFK97/view?usp=sharing
