import React, { useState } from 'react';
import ReactMarkdown from 'react-markdown';
import './ConversationStep.css';


function ConversationItem({children, imageUrl, color = "#888"}) {
    return (
        <div className="Row">
            <div className="Avatar" style={{"backgroundColor": color}}>

            </div>
            <div className="Content">
                {children}
            </div>
        </div>
    )
}

export default function ConversationStep({question, answer}) {
    return (
        <div className="Conversation">
            <div className="Question">
                <ConversationItem color="#5176c0">
                    {question}
                </ConversationItem>
            </div>
            <div className="Answer">
                <ConversationItem color="#FFF">
                    <ReactMarkdown>{answer}</ReactMarkdown>
                </ConversationItem>
            </div>
        </div>
    );
}