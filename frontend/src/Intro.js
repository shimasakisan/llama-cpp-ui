import { useEffect, useState } from 'react';
import './Intro.css';

const messages = [
    <><span className="Main">Experience</span> the power of llama.cpp.</>,
    <><span className="Main">Unleash</span> the potential of LLMs on the edge.</>,
    <><span className="Main">Revolutionize</span> your chat experience with llama.cpp.</>,
    <><span className="Main">Join</span> the llama.cpp revolution.</>,
    <><span className="Main">Empower</span> your chat with llama.cpp.</>,
    <><span className="Main">llama.cpp:</span> inference on the edge.</>,
]


export default function Intro() {

    const [selectedItem, setSelectedItem] = useState(() => {
        return messages[Math.floor(Math.random() * messages.length)];
    });

    useEffect(() => {
        setSelectedItem(messages[Math.floor(Math.random() * messages.length)]);
    }, []);

    return (
        <div className="Intro">
            <div className="Header">{selectedItem}</div>
        </div>
    )
}