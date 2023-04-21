import { useState } from 'react';
import './App.css';
import InputBox from './InputBox';
import TopBar from './TopBar';
import Conversation from './Conversation';
import ConversationStep from './ConversationStep';


const defaultConversation = { title: 'Empty chat', steps: [] };

function App() {
  const [input, setInput] = useState("");
  const [question, setQuestion] = useState("");
  const [answer, setAnswer] = useState("");
  const [isBusy, setIsBusy] = useState(false);
  const [conversation, setConversation] = useState(defaultConversation);
  

  async function handleSubmit() {
    setIsBusy(true);
    setQuestion(input);
    setAnswer("...");
    setInput("");
    let tmpAnswer = "";
    const response = await fetch('http://localhost:8080/stream');
    const reader = response.body.getReader();
    while (true) {
      const { done, value } = await reader.read();
      if (done) {
        setConversation(prev => ({
          ...prev,
          steps: [ ...prev.steps, {question: input, answer: tmpAnswer}]
        }));
        setAnswer("");
        setQuestion("");
        break;
      }
      const text = new TextDecoder().decode(value);
      tmpAnswer += ' ' + text;
      setAnswer(tmpAnswer);
    }
    setIsBusy(false);
  }


  return (
    <div className="App">
      <TopBar title={conversation.title} />
      <Conversation conversation={conversation} />
      {isBusy && <ConversationStep question={question} answer={answer} />}
      <InputBox value={input} onChange={e => setInput(e.target.value)} onSubmit={handleSubmit} />
    </div>
  );
}

export default App;
