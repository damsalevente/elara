# Elara

![image](/docs/256791595_5259425590740247_5532869102347051453_n.png)

## Goal

Provide a software architecture tool which doesn't based on eclipse, and doesn't suck

## What differentiates from other tools

collaboration first: instead of paywall sh**, use it as a "game"

diagram as code: able to describe diagrams in a text based format

good motion: create the "vim motion" for diagrams

"maps from games": create views that looks amazing out of the box


## Requirements

embedded/server builtin: add support for certain things to consider, the template should have items such as: 
    - memory layout
    - runtime behaviour
    - interrupts
    - modules

it shall support reqif format, csv/excel format, matlab format

it shall support uml modeling

it shall be fast

it shall be deployable locally and via cloud

it shall be collaborative, meaning that multiple users can edit the *SAME* diagram

it shall support history (baselines, revert etc.)

the rendering shall be selectable (raylib, web, cli, whatever)

it shall not reinvent a new domain specific language

it shall support uml/sysml rule checks

it shall support light and dark mode 

it shall support custom theming

it shall support exporting to svg/png/pdf 

## What is outside of the scope 

admin/user roles

security: no way I can do that 

plugins and extensibility: no way I can do that, just fork it man


## Build instructions

clone repository then

    brew install zeromq

    cmake .

    cmake --build .
